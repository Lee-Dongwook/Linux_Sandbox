#include <sys/resource.h>

#include "resource.h"

int resource_apply_cpu_limit(unsigned int seconds) {
  struct rlimit limit;

  limit.rlim_cur = (rlim_t)seconds;
  limit.rlim_max = (rlim_t)seconds + 1;

  return setrlimit(RLIMIT_CPU, &limit);
}
