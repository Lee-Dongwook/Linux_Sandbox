#include <stdio.h>

#include "cli.h"
#include "process.h"

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

        case CLI_ACTION_RUN:
          return process_execute(options.command_argv,
                                 options.cpu_limit_seconds,
                                 options.timeout_seconds,
                                 options.memory_limit_mb);
    }

  return 1;
}
