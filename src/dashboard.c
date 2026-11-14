#include <ncurses.h>
#include <stddef.h>
#include <stdlib.h>
#include <sys/types.h>

#include "dashboard.h"
#include "resource_monitor.h"

#define DASHBOARD_MAX_PROCESSES 256

static ResourceSnapshot processes[DASHBOARD_MAX_PROCESSES];
static size_t process_count = 0;
static int selected_index = 0;
static DashboardSortMode sort_mode = SORT_BY_CPU;

void dashboard_set_processes(const ResourceSnapshot *snapshots, size_t count) {
  if (snapshots == NULL) {
    process_count = 0;
    selected_index = 0;
    return;
  }

  if(count > DASHBOARD_MAX_PROCESSES) {
    count = DASHBOARD_MAX_PROCESSES;
  }

  for (size_t i = 0; i < count; i++) {
    processes[i] = snapshots[i];
  }

  process_count = count;

  if (process_count == 0) {
    selected_index = 0;
  } else if (selected_index >= (int)process_count) {
    selected_index = (int)process_count -1;
  }
}

static int compare_cpu(const void *a, const void *b) {
  const ResourceSnapshot *pa = a;
  const ResourceSnapshot *pb = b;

  if (pa->cpu_usage < pb->cpu_usage) {
    return 1;
  }

  if (pa->cpu_usage > pb->cpu_usage) {
    return -1;
  }

  return 0;
}

static int compare_memory(const void *a, const void *b) {
    const ResourceSnapshot *pa = a;
    const ResourceSnapshot *pb = b;

    if (pa->vm_rss_kb < pb->vm_rss_kb) return 1;
    if (pa->vm_rss_kb > pb->vm_rss_kb) return -1;
    return 0;
}

static void render_header(void) {
    attron(COLOR_PAIR(1) | A_BOLD);
    mvprintw(0, 2, "MINI-SANDBOX MONITOR");
    attroff(COLOR_PAIR(1) | A_BOLD);

    attron(COLOR_PAIR(2));
    mvprintw(0, COLS - 12, "LIVE");
    attroff(COLOR_PAIR(2));

    mvhline(1, 0, ACS_HLINE, COLS);
}

static void render_summary(void) {
    unsigned int running = 0;
    unsigned int total_threads = 0;

    mvprintw(3, 2, "SYSTEM SUMMARY");

    mvprintw(
        5, 30,
        "Running: %u",
        running
    );
    mvprintw(
        5, 55,
        "Threads: %u",
        total_threads
    );
    mvhline(7, 0, ACS_HLINE, COLS);
}

static void render_process_table(void) {
    const int header_row = 11;
    const int first_data_row = 13;
    const int available_rows = LINES - first_data_row - 3;

    mvprintw(9, 2, "PROCESS MONITOR");

    mvprintw(
        header_row, 2,
        "%-8s %-8s %10s %12s %12s %8s",
        "PID", "STATE", "CPU%", "RSS(KB)", "VIRT(KB)", "THREADS"
    );

    mvhline(12, 2, ACS_HLINE, COLS - 4);

    if (available_rows <= 0) {
        return;
    }

    size_t display_count = process_count;

    if (display_count > (size_t)available_rows) {
        display_count = (size_t)available_rows;
    }

    for (size_t i = 0; i < display_count; i++) {
      int row = first_data_row + (int)i;

      if ((int)i == selected_index) {
        attron(A_REVERSE);
      }

      mvprintw(row, 2, "%-8d %-8c %9.2f%% %12lu %12lu %8u", processes[i].pid,
               processes[i].state, processes[i].cpu_usage,
               processes[i].vm_rss_kb, processes[i].vm_size_kb,
               processes[i].threads);

      if ((int)i == selected_index) {
        attroff(A_REVERSE);
      }
    }
}

static void render_footer(void) {
  mvhline(LINES - 2, 0, ACS_HLINE, COLS);

  const char *sort_label =
        sort_mode == SORT_BY_CPU ? "CPU DESC" : "RSS DESC";

    mvprintw(
        LINES - 1, 2,
        "UP/DOWN: Select | c: CPU Sort | m: Memory Sort | Sort: %s | q: Quit",
        sort_label
    );
}

void dashboard_render(void) {
    erase();

    render_header();
    render_summary();
    render_process_table();
    render_footer();

    refresh();
}

void dashboard_handle_input(int key) {
  int max_visible = LINES - 13 - 3;

  if (key == KEY_UP && selected_index > 0) {
        selected_index--;
  }

  if (key == KEY_DOWN &&
        selected_index < (int)process_count - 1 && selected_index < max_visible - 1) {
        selected_index++;
  }

  if (key == 'c' || key == 'C') {
    sort_mode = SORT_BY_CPU;
    qsort(processes, process_count, sizeof(ResourceSnapshot), compare_cpu);
    selected_index = 0;
  }

  if (key == 'm' || key == 'M') {
    sort_mode = SORT_BY_MEMORY;
    qsort(processes, process_count, sizeof(ResourceSnapshot), compare_memory);
    selected_index = 0;
  }
}
