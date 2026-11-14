## Trobleshooting: Process Execution Hang and FD Inheritance

### 1. Overview

#### Issue

Mini-Sandbox process execution appreared to hang when running a command under `strace`.

Initial reproduction:

```bash
strace -f -o /tmp/mini-sandbox.trace \
    ./sandbox run -- /bin/echo linux-check
```

Expected output:

```bash
linux-check
```

Actual behavior:

- The command did not appear to complete.
- The terminal became unresponsive.
- Running the command through `timeout` did not initially resolve the issue.

The investigation focused on file descriptor inheritance and the exec error pipe lifecycle.

### 2. Investigation

#### Docker container state

Checked running containers:

```bash
docker ps
```

Inspected container processes:

```bash
docker top <container_id>
docker exec <container_id> ps -ef
```

Observed:

- Multiple containers created by `docker compose` run were still running.

- The inspected containers showed `/bin/bash` as their main process.

- No active Mini-Sandbox process was visible at the time of inspection.

This suggested that the previously interrupted commands had already terminated, rather than proving that Docker itself was frozen.

#### Verify strace independently

Tested strace against a simple process:

```bash
strace /bin/echo hello
```

Tested timeout behavior:

```bash
timeout 3s strace /bin/sleep 10
echo "exit code: $?"
```

Observed:

- strace successfully traced `/bin/sleep`.

- timeout terminated the command after three seconds.

- Exit `code 124` indicated that the timeout period had expired.

This established that basic strace and timeout functionality worked inside the container.

### 3. FD Inheritance and Exec Error Pipe

Mini-Sandbox uses an error pipe to distinguish successful process execution from `execvp()` failures.

#### Expected lifecycle

The intended flow is:

- Parent creates an error pipe.

- The write end receives `FD_CLOEXEC`.

- Parent forks a child process.

- Child closes the pipe's read end.

- Child closes inherited file descriptors except the error pipe's write end.

- Child executes the requested program using `execvp()`.

- On successful execution, `FD_CLOEXEC` automatically closes the write end.

- Parent receives `EOF` from the read end.

- If `execvp()` fails, child writes errno to the pipe before exiting.

#### Important implementation details

The error pipe is created with:

```c
pipe(pipe_fd);
```

The write end is marked close-on-exec:

```c
int flags = fcntl(pipe_fd[1], F_GETFD);

fcntl(
    pipe_fd[1],
    F_SETFD,
    flags | FD_CLOEXEC
);
```

The child preserves the error pipe's write descriptor while closing other inherited descriptors:

```c
close(exec_error_pipe[0]);

if (close_inherited_fds(exec_error_pipe[1]) == -1) {
    _exit(126);
}
```

The parent closes its copy of the write descriptor:

```c
close(exec_error_pipe[1]);
```

This is important because an unintended open write descriptor can prevent the parent from receiving EOF.

### 4. Verification

After reviewing and correcting the FD handling and exec error pipe behavior, verify execution in the following order.

#### Normal execution

```bash
./sandbox run -- /bin/echo linux-check
```

Expected:

```bash
linux-check
```

#### Trace process creation and execution

```bash
strace -f -tt \
 -o /tmp/mini-sandbox.trace \
 ./sandbox run -- /bin/echo linux-check
```

#### Inspect trace output

```bash
tail -n 50 /tmp/mini-sandbox.trace
```

Inspect the following operations:

- `clone, clone3, or fork`: process creation

- `setpgid`: process group configuration

- `close`: file descriptor cleanup

- `execve`: execution of the target program

- `read and write`: exec error pipe communication

- `wait4` or `equivalent`: child process collection

Note that libc functions do not necessarily map one-to-one to Linux syscall names. For example, `waitpid()` may appear as `wait4()` in the trace.

### 5. Remaining Considerations

Further improvements worth considering:

- Use pipe2(O_CLOEXEC) on Linux to create close-on-exec descriptors atomically.

- Handle close() errors where meaningful.

- Ensure child setup failures are reported through the same error channel, not only execvp() failures.

- Add integration tests for successful execution, invalid executable paths, and child setup failures.

- Consider descriptor-closing strategies that avoid iterating over a potentially large RLIMIT_NOFILE range.
