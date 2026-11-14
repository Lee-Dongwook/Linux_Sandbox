#include <string.h>
#include <ncurses.h>
#include <unistd.h>

#include "tui.h"
#include "dashboard.h"
#include "resource_monitor.h"
#include "system_inspector.h"
#include "system_monitor.h"

#define MONITOR_MAX_PROCESSES 256

int tui_run(void) {
    if (initscr() == NULL) {
        return -1;
    }

    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    curs_set(0);
    timeout(1000);

    if (has_colors()) {
        start_color();
        use_default_colors();

        init_pair(1, COLOR_GREEN, -1);
        init_pair(2, COLOR_CYAN, -1);
        init_pair(3, COLOR_WHITE, -1);
    }

    ResourceSnapshot previous[MONITOR_MAX_PROCESSES];
    ResourceSnapshot current[MONITOR_MAX_PROCESSES];
    SystemSnapshot previous_system;
    SystemSnapshot current_system;
    SystemMetrics system_metrics;

    size_t previous_count = 0;
    size_t current_count = 0;

    int has_previous_system = 0;

    long clock_ticks = sysconf(_SC_CLK_TCK);

    int running = 1;

    while (running) {
      current_count = 0;

      if (system_inspector_collect(&current_system) == 0) {
        const SystemSnapshot *previous_ptr =
            has_previous_system ? &previous_system : NULL;

        if (system_monitor_calculate(previous_ptr, &current_system,
                                     &system_metrics) == 0) {
          dashboard_set_system_metrics(&system_metrics);

          previous_system = current_system;
          has_previous_system = 1;
        }
      }

        int status = resource_monitor_collect_all(
            current, MONITOR_MAX_PROCESSES, &current_count);

        if (status == 0) {
           if (previous_count > 0 && clock_ticks > 0) {
             resource_monitor_update_cpu_usage(previous,
                                               previous_count, current, current_count, clock_ticks);
           }

           dashboard_set_processes(current, current_count);

           memcpy(
             previous,
             current,
             current_count * sizeof(ResourceSnapshot)
           );

           previous_count = current_count;
        }

        dashboard_render();

        /* Keys and KEY_RESIZE only re-render; data is re-sampled once
           getch() times out, so a resize burst doesn't spam samples.
           ponytail: continuous key presses delay sampling; switch to a
           monotonic-clock deadline if that ever matters. */
        int key;
        while ((key = getch()) != ERR) {
            if (key == 'q' || key == 'Q') {
                running = 0;
                break;
            }

            dashboard_handle_input(key);
            dashboard_render();
        }
    }

    endwin();

    return 0;
}
