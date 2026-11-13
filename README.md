# Linux_Sandbox

A lightweight process execution controller written in C.

The project focuses on understanding Linux system programming concepts through hands-on implementation, including process lifecycle management, system calls, resource limits, and execution control.

- **Project Status**: MVP — Process Execution and Resource Control
- **Language**: C, Python
- **Build**: GCC, Make
- **Testing:** pytest, GitHub Actions

## 1. Overview

Running an external command involves more than simply invoking a process.

A process execution controller needs to manage:

-Process creation and termination

- Exit status collection

- CPU and memory consumption

- Execution time limits

- File descriptor inheritance

- Child process termination

- Execution environment configuration

- Execution history and logging

implements these capabilities using Unix/Linux system interfaces.

The current implementation provides a controlled execution environment for individual commands, with basic resource restrictions and process lifecycle management.

## 2. Features

### Process Management

- Create child processes using `fork()`

- Execute external commands using `execvp()`

- Collect process status using `waitpid()`

- Handle normal exits and signal-based termination

- Distinguish command execution failures using a close-on-exec error pipe

### Execution Monitoring

- Wall-clock timeout using clock_gettime(`CLOCK_MONOTONIC`)

- Non-blocking process monitoring with waitpid(`WNOHANG`)

- Process-group creation using `setpgid()`

- Process-group termination using `kill(-pgid, SIGKILL)`

### Execution Environment

- Custom working directory using `--cwd`

- Close inherited file descriptors before executing the target command

- Preserve standard input, output, and error descriptors

### Security-Related Restrictions

- Disable core dumps using `RLIMIT_CORE`

- Apply `PR_SET_NO_NEW_PRIVS` on Linux

- Keep platform-specific behavior separated through conditional compilation

### Structured Logging

Execution results are recorded in JSON Lines format.

Execution duration in millisecondsExecution results are recorded in JSON Lines format.

Each record contains:

- Command and arguments

- Exit code

- Timeout status

- Execution duration in milliseconds

```json
{
  "command": ["echo", "Hello"],
  "exit_code": 0,
  "timed_out": false,
  "duration_ms": 12
}
```

## 3. Installation

### Requirements

- GCC or a compatible C compiler

- GNU Make or compatible Make implementation

- Python 3.12+

- pytest

### Build

```bash
git clone https://github.com/Lee-Dongwook/Mini-Sandbox.git
cd Mini-Sandbox
```

```bash
make clean
make
```

The build produces the `sandbox` executable in the project root.

### Install Test Dependencies

```bash
python -m pip install -r requirements-dev.txt
```

## 4. Usage

### Basic Execution

```bash
./sandbox run --echo "Hello, Sandbox!"
```

### CPU Time Limit

```bash
./sandbox run --cpu 2 -- python3 -c "while True: pass"
```

### Wall-Clock Timeout

```bash
./sandbox run --timeout 3 -- sleep 10
```

### Memory Limit

```bash
./sandbox run --memory 64 -- python3 -c \
"data = bytearray(128 * 1024 * 1024)"
```

### File Size Limit

```bash
./sandbox run --file-size 1 -- python3 -c \
"open('output.bin', 'wb').write(b'0' * (2 * 1024 * 1024))"
```

### File Descriptor Limit

```bash
./sandbox run --nofile 32 -- python3 -c \
"import os; print(os.listdir('/proc/self/fd'))"
```

### Custom Working Directory

```bash
./sandbox run --cwd /tmp -- python3 -c \
"import os; print(os.getcwd())"
```

### Combine Options

```bash
./sandbox run \
  --cpu 2 \
  --timeout 5 \
  --memory 128 \
  --file-size 10 \
  --nofile 64 \
  --cwd /tmp \
  -- \
  python3 -c "print('Execution completed')"
```

## 5. Testing

uses pytest for integration testing.

The tests cover:

- Successful command execution

- Non-zero exit codes

- Command execution failures

- CPU resource limits

- Memory limits

- File size restrictions

- File descriptor limits

- Execution timeouts

- Process-group termination

- Custom working directories

- JSONL logging

Run the complete test suite:

```bash
make clean && make
python -m pytest tests/ -v
```

## 6. Limitations and Security Considerations.

This is an educational process execution controller, not a hardened security sandbox.

The current implementation does not provide complete isolation against malicious workloads.

Known limitations include:

- No filesystem namespace isolation

- No network namespace isolation

- No seccomp-BPF syscall filtering

- No cgroups-based resource accounting or enforcement

- Process-group termination does not guarantee containment of descendants that escape their process group

- RLIMIT_AS limits virtual address space, not physical RAM usage

- Resource limits do not provide complete protection against kernel-level vulnerabilities

- PR_SET_NO_NEW_PRIVS is applied only on Linux; the current macOS implementation skips this operation

- The execution environment still operates under the invoking user's permissions

**Do not use the current implementation to execute untrusted production workloads.**
