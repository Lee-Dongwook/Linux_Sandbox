
#include <stdio.h>
#include <errno.h>

#include "cli.h"
#include "process.h"
#include "tui.h"

int main(int argc, char *argv[]) {
    CliOptions options;

    if (cli_parse(argc, argv, &options) != 0) {
        return 1;
    }

    switch (options.action) {
        case CLI_ACTION_HELP:
            cli_print_help(argv[0]);
            return 0;

        case CLI_ACTION_VERSION:
            printf("Mini-Sandbox version 0.1.0\n");
            return 0;

        case CLI_ACTION_TUI:
            return tui_run() == 0 ? 0 : 1;

        case CLI_ACTION_RUN: {
            ProcessResult result;

            int status = process_execute(
                options.command_argv,
                options.cpu_limit_seconds,
                options.timeout_seconds,
                options.memory_limit_mb,
                options.file_size_limit_mb,
                options.nofile_limit,
                options.working_directory,
                &result
            );

            if (status == -1) {
                perror("process_execute");
                return 1;
            }

            switch (result.termination_reason) {
                case PROCESS_TERMINATION_EXIT:
                    return result.exit_code;

                case PROCESS_TERMINATION_SIGNAL:
                    return 128 + result.signal_number;

                case PROCESS_TERMINATION_TIMEOUT:
                    return 124;

                case PROCESS_TERMINATION_EXEC_ERROR:
                    errno = result.exec_errno;
                    perror("execvp");
                    return 127;
            }

            return 1;
        }
    }
    return 1;
}
