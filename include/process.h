#ifndef PROCESS_H
#define PROCESS_H

int process_execute(char *const command_argv[],
                    unsigned int cpu_limit_seconds,
                    unsigned int timeout_seconds);

#endif
