#include <ncurses.h>
#include <stddef.h>
#include <stdlib.h>
#include <sys/types.h>

#include "dashboard.h"
#include "resource_monitor.h"

#define DASHBOARD_MAX_PROCESSES 256
#define CPU_HISTORY_LENGTH 60

static ResourceSnapshot processes[DASHBOARD_MAX_PROCESSES];
static size_t process_count = 0;
static int selected_index = 0;
static DashboardSortMode sort_mode = SORT_BY_CPU;
static int compare_cpu(const void *a, const void *b);
static int compare_memory(const void *a, const void *b);
static double cpu_history[CPU_HISTORY_LENGTH];
static size_t cpu_history_count = 0;
static size_t cpu_history_next = 0;
static pid_t cpu_history_pid = -1;

static void reset_cpu_history(pid_t pid) {
  cpu_history_count = 0;
  cpu_history_next = 0;
  cpu_history_pid = pid;
}

static void record_cpu_history(void) {
  if (process_count == 0) {
    reset_cpu_history(-1);
    return;
  }

  if (selected_index < 0 || selected_index >= (int)process_count) {
    return;
  }

  pid_t selected_pid = processes[selected_index].pid;

  if (cpu_history_pid != selected_pid) {
      reset_cpu_history(selected_pid);
  }

  cpu_history[cpu_history_next] = processes[selected_index].cpu_usage;

  cpu_history_next = (cpu_history_next + 1) % CPU_HISTORY_LENGTH;

  if (cpu_history_count < CPU_HISTORY_LENGTH) {
      cpu_history_count++;
  }
}

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

  if (sort_mode == SORT_BY_CPU) {
    qsort(processes, process_count,
          sizeof(ResourceSnapshot), compare_cpu
    );
  } else {
    qsort(processes, process_count, sizeof(ResourceSnapshot), compare_memory);
  }

  record_cpu_history();
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

    for (size_t i = 0; i < process_count; i++) {
      if (processes[i].state == 'R') {
            running++;
      }

       total_threads += processes[i].threads;
    }

    mvprintw(3, 2, "SYSTEM SUMMARY");

    mvprintw(
        5, 5,
        "Processes: %zu",
        process_count
    );

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

static void render_bar(int row, int col, double value, double max_value,
                       int width) {
  if (max_value <= 0 || width <= 0) {
    return;
  }

  if (value < 0) {
    value = 0;
  }

  if (value > max_value) {
    value = max_value;
  }

  int filled = (int)((value / max_value) * width);

  for (int i = 0; i < width; i++) {
    mvaddch(row, col + i, i < filled ? '#' : '-');
  }
}

static void render_cpu_history(void) {
  const int chart_top = 11;
  const int chart_bottom = 17;
  const int chart_left = 8;

  int chart_width = COLS - chart_left - 2;

  if (chart_width > CPU_HISTORY_LENGTH) {
    chart_width = CPU_HISTORY_LENGTH;
  }

  if (chart_width <= 0) {
    return;
  }

  if (cpu_history_count == 0) {
        mvprintw(9, 2, "CPU HISTORY - Waiting for samples");
        return;
  }

  mvprintw(
        9, 2,
        "CPU HISTORY - PID: %d (Last 60 Samples)",
        cpu_history_pid
  );

  mvprintw(chart_top, 2, "100%%");
  mvprintw(14, 2, " 50%%");
  mvprintw(chart_bottom, 2, "  0%%");

  for (int x = chart_left; x < chart_left + chart_width; x++) {
    mvaddch(chart_top, x, ACS_HLINE);
    mvaddch(14, x, ACS_HLINE);
    mvaddch(chart_bottom, x, ACS_HLINE);
  }

  size_t display_count = cpu_history_count;

  if (display_count > (size_t)chart_width) {
        display_count = (size_t)chart_width;
  }

  size_t oldest_index =
      (cpu_history_next + CPU_HISTORY_LENGTH - cpu_history_count) %
      CPU_HISTORY_LENGTH;

  size_t start_offset = cpu_history_count - display_count;

  int previous_y = -1;

  for (size_t i = 0; i < display_count; i++) {
    size_t history_index = (oldest_index + start_offset + i) % CPU_HISTORY_LENGTH;

    double value = cpu_history[history_index];

    if (value < 0.0) {
      value = 0.0;
    }

    if (value > 100.0) {
      value = 100.0;
    }

    int y = chart_bottom - (int)((value / 100.0) * (chart_bottom - chart_top));
    int x = chart_left + (int)i;

    if (previous_y >= 0) {
      if (y == previous_y) {
        mvaddch(y, x, ACS_HLINE);
      } else if (y < previous_y) {
        mvaddch(y, x, '/');
      } else {
        mvaddch(y, x, '\\');
      }
    }

    mvaddch(y, x, '*');

    previous_y = y;
  }
}

static void render_process_table(void) {
    const int header_row = 21;
    const int first_data_row = 23;
    const int available_rows = LINES - first_data_row - 3;

    const int cpu_bar_width = 20;
    const int memory_bar_width = 20;

    unsigned long max_rss = 0;

    mvprintw(19, 2, "PROCESS MONITOR");

    for (size_t i = 0; i < process_count; i++) {
      if (processes[i].vm_rss_kb > max_rss) {
        max_rss = processes[i].vm_rss_kb;
      }
    }

    mvprintw(header_row, 2, "PID");
    mvprintw(header_row, 11, "STATE");
    mvprintw(header_row, 20, "CPU%%");
    mvprintw(header_row, 30, "CPU USAGE");
    mvprintw(header_row, 54, "RSS(KB)");
    mvprintw(header_row, 68, "MEMORY USAGE");
    mvprintw(header_row, 93, "THREADS");

    mvhline(22, 2, ACS_HLINE, COLS - 4);

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

      mvprintw(row, 2, "%-8d", processes[i].pid);
      mvprintw(row, 11, "%-8c", processes[i].state);
      mvprintw(row, 20, "%7.2f%%", processes[i].cpu_usage);
      render_bar(
            row,
            30,
            processes[i].cpu_usage,
            100.0,
            cpu_bar_width
        );
      mvprintw(row, 54, "%-10lu", processes[i].vm_rss_kb);
      render_bar(
            row,
            68,
            (double)processes[i].vm_rss_kb,
            (double)max_rss,
            memory_bar_width
        );
      mvprintw(row, 93, "%-8u", processes[i].threads);

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
    render_cpu_history();
    render_process_table();
    render_footer();

    refresh();
}

void dashboard_handle_input(int key) {
  int max_visible = LINES - 23 - 3;

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
