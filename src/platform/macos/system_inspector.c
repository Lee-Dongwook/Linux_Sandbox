#include <errno.h>

#include "system_inspector.h"

/* System Inspector is /proc-based and Linux-only (see platform/linux/).
 * macOS needs sysctl/host_statistics; until implemented, report unsupported
 * rather than returning fabricated metrics. */
int system_inspector_collect(SystemSnapshot *snapshot) {
    (void)snapshot;
    errno = ENOSYS;
    return -1;
}
