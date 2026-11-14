#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <limits.h>

#include "cli.h"

void cli_print_help(const char *program_name) {
    printf("Mini-Sandbox\n\n");

    printf("Usage:\n");
    printf("  %s --help\n", program_name);
    printf("  %s --version\n", program_name);
    printf("  %s --tui\n", program_name);
    printf(
    "  %s run [--cpu seconds] [--timeout seconds] "
    "[--memory MB] [--file-size MB] [--nofile count] -- [command] [args...] [--cwd directory]\n",
    program_name
    );
}

static int parse_cpu_limit(const char *value,
                           unsigned int *seconds
) {
  char *end;
  unsigned long parsed;

  errno = 0;
  parsed = strtoul(value, &end, 10);

  if (errno != 0 || end == value || *end != '\0' || parsed == 0 ||
      parsed > UINT_MAX) {
    return -1;
  }

  *seconds = (unsigned int)parsed;
  return 0;
}

static int parse_ulong_limit(
  const char *value,
  unsigned long *result
) {
  char *end;
  unsigned long parsed;

  errno = 0;
  parsed = strtoul(value, &end, 10);

  if (errno != 0 || end == value || *end != '\0' || parsed == 0) {
        return -1;
  }

  *result = parsed;
  return 0;
}

int cli_parse(int argc, char *argv[], CliOptions *options) {
    if (options == NULL || argv == NULL) {
        return -1;
    }

    options->command_argv = NULL;
    options->cpu_limit_seconds = 0;
    options->timeout_seconds = 0;
    options->memory_limit_mb = 0;
    options->file_size_limit_mb = 0;
    options->nofile_limit = 0;
    options->working_directory = NULL;

    if (argc < 2) {
        options->action = CLI_ACTION_HELP;
        return 0;
    } 

    if (strcmp(argv[1], "--help") == 0) {
        options->action = CLI_ACTION_HELP;
        return 0;
    }

    if (strcmp(argv[1], "--version") == 0) {
        options->action = CLI_ACTION_VERSION;
        return 0;
    }

    if (strcmp(argv[1], "--tui") == 0) {
        options->action = CLI_ACTION_TUI;
        return 0;
    }

    if (strcmp(argv[1], "run") == 0) {
      int index = 2;

      while (index < argc && strcmp(argv[index], "--") != 0) {
        if (strcmp(argv[index], "--cpu") == 0) {
          if (index + 1 >= argc) {
            fprintf(stderr, "Missing value for --cpu\n");
            return -1;
          }

          if (parse_cpu_limit(argv[index + 1], &options->cpu_limit_seconds) !=
              0) {
            fprintf(stderr, "Invalid CPU limit\n");
            return -1;
          }
          index += 2;
          continue;
        }

        if (strcmp(argv[index], "--timeout") == 0) {
          if (index + 1 >= argc) {
            fprintf(stderr, "Missing value for --timeout\n");
            return -1;
          }

          if (parse_cpu_limit(argv[index + 1], &options->timeout_seconds) !=
              0) {
            fprintf(stderr, "Invalid timeout\n");
            return -1;
          }
          index += 2;
          continue;
        }

        if (strcmp(argv[index], "--memory") == 0) {
            if (index + 1 >= argc) {
              fprintf(stderr, "Missing value for --memory\n");
              return -1;
            }

            if (parse_ulong_limit(argv[index + 1], &options->memory_limit_mb) !=
                0) {
              fprintf(stderr, "Invalid memory limit\n");
              return -1;
            }

            index += 2;
            continue;
        }

        if (strcmp(argv[index], "--file-size") == 0) {
            if (index + 1 >= argc) {
                fprintf(stderr, "Missing value for --file-size\n");
                return -1;
            }

            if (parse_ulong_limit(
                    argv[index + 1],
                  &options->file_size_limit_mb
                ) != 0) {
                fprintf(stderr, "Invalid file size limit\n");
                return -1;
            }

            index += 2;
            continue;
        }

        if (strcmp(argv[index], "--nofile") == 0) {
          if (index + 1 >= argc) {
            fprintf(stderr, "Missing value for --nofile\n");
            return -1;
          }

          if (parse_cpu_limit(argv[index + 1], &options->nofile_limit) != 0) {
            fprintf(stderr, "Invalid file descriptor limit\n");
            return -1;
          }

          index += 2;
          continue;
        }

        if(strcmp(argv[index], "--cwd") == 0) {
          if (index + 1 >= argc || strcmp(argv[index + 1], "--") == 0) {
            fprintf(stderr, "Missing value for --cwd\n");
            return -1;
          }

          options->working_directory = argv[index + 1];

          index += 2;
          continue;
        }

        fprintf(stderr, "Unknown option: %s\n", argv[index]);
        return -1;
      }


      if (index >= argc || index + 1 >= argc) {
            fprintf(
                stderr,
                "Usage: %s run [--cpu seconds] [--timeout seconds] -- [command] [args...]\n",
                argv[0]
            );
            return -1;
        }

        options->action = CLI_ACTION_RUN;
        options->command_argv = &argv[index + 1];

      return 0;
    }

    fprintf(stderr, "Unknown command: %s\n", argv[1]);

    return -1;
}
