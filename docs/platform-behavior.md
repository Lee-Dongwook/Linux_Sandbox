## Platform-specific Behavior

### 1. Supported Development Environments

Mini-Sandbox is developed and tested in the following environments.

| Environment | Operating System                                 | Architecture | Purpose                                               |
| ----------- | ------------------------------------------------ | ------------ | ----------------------------------------------------- |
| Local       | macOS Darwin 25.6.0                              | ARM64        | Development and editing                               |
| Container   | Ubuntu 24.04 userspace on LinuxKit kernel 7.0.12 | ARM64        | Linux-specific implementation and integration testing |

The Linux environment runs through Docker Desktop. Its kernel is provided by LinuxKit, while the container uses Ubuntu 24.04 userspace.

Linux-specific behavior must be verified in the Linux container rather than inferred from macOS results.

### 2. Process Management

Mini-Sandbox uses standard process-management interfaces:

- `fork()`

- `execvp()`

- `waitpid()`

- `setpgid()`

- `kill()`

Although these interfaces are available across Unix-like systems, their underlying kernel implementations and observable behavior may differ.

For example, Linux system call traces may show `clone()` or `clone3()` for process creation and wait4() for child process collection.

The exact syscall sequence is platform-dependent.

### 3. Resource Limits

Mini-Sandbox currently uses `setrlimit()` to apply resource restrictions.

| Resource      | Limit         | Purpose                                                 |
| ------------- | ------------- | ------------------------------------------------------- |
| CPU time      | RLIMIT_CPU    | Limits CPU time consumed by the child process           |
| Address space | RLIMIT_AS     | Limits virtual address space                            |
| File size     | RLIMIT_FSIZE  | Limits the maximum size of files created by the process |
| Open files    | RLIMIT_NOFILE | Limits the number of open file descriptors              |
| Core dump     | RLIMIT_CORE   | Disables core dump generation                           |

### 4. Verification

Build and run tests in the Ubuntu 24.04 container:

```bash
docker compose build
docker compose run --rm mini-sandbox
make clean
make
python3 -m pytest tests/ -v
```

For syscall inspection on Linux:

```bash
strace -f -tt \
  -o /tmp/mini-sandbox.trace \
  ./sandbox run -- /bin/echo linux-check
```

### 5. Known Limitations

- Resource limit behavior has not been established as identical across macOS and Linux.

- PR_SET_NO_NEW_PRIVS is not applied on macOS.

- No namespace-based filesystem or network isolation is available.

- No seccomp-BPF filtering is available.

- No cgroups v2 resource management is available.

- Linux syscall traces are not directly portable to macOS.

- `/proc`-based system and process monitoring is Linux-only (see section 6).

### 6. Cross-platform Build and Platform Boundary

Compiling on a platform is not the same as supporting it at runtime. Both platforms compile from the same sources; their runtime support differs.

| Platform | Compiler    | cJSON source                        | Platform sources      |
| -------- | ----------- | ----------------------------------- | --------------------- |
| macOS    | Apple Clang | `brew --prefix cjson` (include/lib) | `src/platform/macos/` |
| Linux    | GCC         | system (`libcjson-dev`)             | `src/platform/linux/` |

The Makefile picks the directory based on `uname -s`. Code in `src/` is shared. Code that implements an OS-specific interface lives under `src/platform/<os>/` and keeps the header in `include/`.

#### Linux-specific components

| Component | Dependency | macOS status |
| --------- | ---------- | ------------ |
| System Inspector (`src/platform/linux/system_inspector.c`) | `/proc/stat`, `/proc/meminfo`, `/proc/loadavg`, `/proc/uptime` | Not implemented. The stub `system_inspector_collect()` fails with `ENOSYS`. No metrics are faked. |
| Process Inspector (`src/inspector.c`) | `/proc`, `/proc/<pid>/status`, `/proc/<pid>/stat` | Compiles, but fails at runtime because `/proc` does not exist |
| `close_range` fd cleanup (`src/process.c`) | `SYS_close_range` | Uses the portable fallback path |
| `PR_SET_NO_NEW_PRIVS` (`src/resource.c`) | `prctl()` | No-op |

As a result, on macOS the IPC server (`sandbox --server`) exits with `Failed to initialize system snapshot`, and TUI system metrics are unavailable. A future macOS System Inspector (e.g. `sysctl` / `host_statistics64`) belongs in `src/platform/macos/system_inspector.c`.

#### IPC SIGPIPE handling

The server must not be killed by `SIGPIPE` when a client disconnects before it reads its response. That write has to fail with `EPIPE` instead. This is handled in one place, `ipc_disable_sigpipe()` / `IPC_SEND_FLAGS` in `src/ipc_server.c`:

- Linux: each `send()` passes `MSG_NOSIGNAL`.
- macOS: each accepted socket gets `SO_NOSIGPIPE` via `setsockopt()`. This requires `_DARWIN_C_SOURCE`, because `_POSIX_C_SOURCE` hides it.

The NDJSON framing, partial read/write loops and the protocol itself are the same on both platforms.
