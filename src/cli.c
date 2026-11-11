#include <stdio.h>
#include <string.h>

#include "cli.h"

void cli_print_help(const char *program_name) {
    printf("Mini-Sandbox\n\n");

    printf("Usage:\n");
    printf("  %s --help\n", program_name);
    printf("  %s --version\n", program_name);
    printf("  %s run -- [command] [args...]\n", program_name);
}

int cli_parse(int argc, char *argv[], CliOptions *options) {
    if (options == NULL || argv == NULL) {
        return -1;
    }

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

    if (strcmp(argv[1], "run") == 0) {
        options->action = CLI_ACTION_RUN;
        return 0;
    }

    fprintf(stderr, "Unknown command: %s\n", argv[1]);

    return -1;
}
