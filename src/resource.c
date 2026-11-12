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
