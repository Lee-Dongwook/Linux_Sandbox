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
