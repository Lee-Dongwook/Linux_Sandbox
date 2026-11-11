#ifndef CLI_H
#define CLI_H

typedef enum {
  CLI_ACTION_HELP,
  CLI_ACTION_VERSION,
  CLI_ACTION_RUN,
} CliAction;

typedef struct {
  CliAction action;
  char **command_argv;
  unsigned int cpu_limit_seconds;
} CliOptions;

int cli_parse(int argc, char *argv[], CliOptions *options);

void cli_print_help(const char *program_name);

#endif
