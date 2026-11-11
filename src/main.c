#include <stdio.h>

#include "cli.h"

int main(int argc, char *argv[]) {
  CliOptions options;

  if (cli_parse(argc, argv, &options) != 0) {
    return 1;
  }

  switch (options.action) {
        case CLI_ACTION_HELP:
            cli_print_help(argv[0]);
            break;

        case CLI_ACTION_VERSION:
            printf("Mini-Sandbox version 0.1.0\n");
            break;

        case CLI_ACTION_RUN:
            printf("Run command is not implemented yet.\n");
            break;
  }
  return 0;
}
