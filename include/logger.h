#ifndef LOGGER_H
#define LOGGER_H

int logger_log_execution(char *const command_argv[], int exit_code,
                         int timed_out,
                         long duration_ms
);

#endif
