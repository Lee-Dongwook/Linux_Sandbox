#include <stdio.h>
#include <time.h>

#include "logger.h"

#define LOG_FILE "sandbox.jsonl"

static int write_json_string(FILE *file, const char *value) {
    if (fputc('"', file) == EOF) {
        return -1;
    }

    while (*value != '\0') {
      unsigned char ch = (unsigned char)*value++;

      switch (ch) {
      case '"':
        if (fputs("\\\"", file) == EOF) {
          return -1;
        }
        break;

      case '\\':
        if (fputs("\\\\", file) == EOF) {
          return -1;
        }
        break;

      case '\n':
        if (fputs("\\n", file) == EOF) {
          return -1;
        }
        break;

        case '\r':
            if (fputs("\\r", file) == EOF) {
                    return -1;
            }
            break;

        case '\t':
            if (fputs("\\t", file) == EOF) {
                return -1;
            }
            break;

        default:
          if (ch < 0x20) {
            if (fprintf(file, "\\u%04x", ch) < 0) {
                    return -1;
                }
          } else if (fputc(ch, file) == EOF) {
            return -1;
          }
      }
    }
    return fputc('"', file) == EOF ? -1 : 0;
}

int logger_log_execution(char *const command_argv[], int exit_code,
                         int timed_out, long duration_ms) {
  if (command_argv == NULL || command_argv[0] == NULL) {
    return -1;
  }

  FILE *file = fopen(LOG_FILE, "a");

  if (file == NULL) {
    return -1;
  }

  int result = 0;

  if (fputs("{\"command\":[", file) == EOF) {
    result = -1;
    goto cleanup;
  }

  for (int i = 0; command_argv[i] != NULL; i++) {
    if (i > 0 && fputc(',', file) == EOF) {
      result = -1;
      goto cleanup;
    }

    if (write_json_string(file, command_argv[i]) != 0) {
      result = -1;
      goto cleanup;
    }
  }

  if (fprintf(file,
              "],\"exit_code\":%d,\"timed_out\":%s,"
              "\"duration_ms\":%ld}\n",
              exit_code, timed_out ? "true" : "false", duration_ms) < 0) {
    result = -1;
    goto cleanup;
  }

  if (fflush(file) == EOF) {
    result = -1;
  }

  cleanup:
    if (fclose(file) == EOF) {
        result = -1;
    }

    return result;
}
