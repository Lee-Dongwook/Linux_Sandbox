#include <sys/resource.h>

#include "resource.h"

int resource_apply_cpu_limit(unsigned int seconds) {
  struct rlimit limit;

  limit.rlim_cur = (rlim_t)seconds;
  limit.rlim_max = (rlim_t)seconds + 1;

  return setrlimit(RLIMIT_CPU, &limit);
}

int resource_apply_memory_limit(unsigned long megabytes) {
  struct rlimit limit;

  rlim_t bytes = (rlim_t)megabytes * 1024 * 1024;

  limit.rlim_cur = bytes;
  limit.rlim_max = bytes;

  return setrlimit(RLIMIT_AS, &limit);
}

int resource_apply_file_size_limit(unsigned long megabytes)
{
  struct rlimit limit;

  if (getrlimit(RLIMIT_FSIZE, &limit) == -1) {
    return -1;
  }

  rlim_t bytes = (rlim_t)megabytes * 1024 * 1024;

  if (limit.rlim_max != RLIM_INFINITY && bytes > limit.rlim_max) {
      bytes = limit.rlim_max;
  }

  limit.rlim_cur = bytes;

  return setrlimit(RLIMIT_FSIZE, &limit);
}

int resource_apply_nofile_limit(unsigned int max_files)
{
    struct rlimit limit;

    if (getrlimit(RLIMIT_NOFILE, &limit) == -1) {
        return -1;
    }

    rlim_t requested = (rlim_t)max_files;

    if (limit.rlim_max != RLIM_INFINITY &&
        requested > limit.rlim_max) {
        requested = limit.rlim_max;
    }

    limit.rlim_cur = requested;

    return setrlimit(RLIMIT_NOFILE, &limit);
}
