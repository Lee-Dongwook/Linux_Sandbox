#ifndef DASHBOARD_H
#define DASHBOARD_H

#include <stddef.h>
#include "resource_monitor.h"
#include "system_monitor.h"

void dashboard_set_processes(const ResourceSnapshot *snapshots, size_t count);
void dashboard_set_system_metrics(const SystemMetrics *metrics);
void dashboard_render(void);
void dashboard_handle_input(int key);

typedef enum {
  SORT_BY_CPU,
  SORT_BY_MEMORY
} DashboardSortMode;

#endif
