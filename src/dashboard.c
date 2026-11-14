#include <ncurses.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>

#include "dashboard.h"
#include "resource_monitor.h"
#include "system_monitor.h"

#define DASHBOARD_MAX_PROCESSES 256
#define CPU_HISTORY_LENGTH 60
#define MIN_TABLE_ROWS 3
#define MIN_COLS 40
#define FOOTER_ROWS 2

static ResourceSnapshot processes[DASHBOARD_MAX_PROCESSES];
static size_t process_count = 0;
static int selected_index = 0;   /* absolute index into processes[] */
static int scroll_offset = 0;    /* index of first visible table row */
static int visible_rows = 1;     /* table rows that fit, set by last render */
static DashboardSortMode sort_mode = SORT_BY_CPU;
static int compare_cpu(const void *a, const void *b);
static int compare_memory(const void *a, const void *b);
static double cpu_history[CPU_HISTORY_LENGTH];
static size_t cpu_history_count = 0;
static size_t cpu_history_next = 0;
static pid_t cpu_history_pid = -1;
static SystemMetrics system_metrics;
static int has_system_metrics = 0;

/* Clamp selection into [0, count) and scroll so it stays inside a
   viewport of `rows` lines. */
static void clamp_view(int rows) {
  int count = (int)process_count;

  if (rows < 1) {
    rows = 1;
  }

  if (count == 0) {
    selected_index = 0;
    scroll_offset = 0;
    return;
  }

  if (selected_index < 0) selected_index = 0;
  if (selected_index >= count) selected_index = count - 1;

  if (selected_index < scroll_offset) {
    scroll_offset = selected_index;
  }
  if (selected_index >= scroll_offset + rows) {
    scroll_offset = selected_index - rows + 1;
  }

  int max_offset = count > rows ? count - rows : 0;
  if (scroll_offset > max_offset) scroll_offset = max_offset;
  if (scroll_offset < 0) scroll_offset = 0;
}

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

void dashboard_set_system_metrics(const SystemMetrics *metrics) {
  if (metrics == NULL) {
    has_system_metrics = 0;
    return;
  }

  system_metrics = *metrics;
  has_system_metrics = 1;
}

void dashboard_set_processes(const ResourceSnapshot *snapshots, size_t count) {
  if (snapshots == NULL) {
    process_count = 0;
    selected_index = 0;
    scroll_offset = 0;
    return;
  }

  if(count > DASHBOARD_MAX_PROCESSES) {
    count = DASHBOARD_MAX_PROCESSES;
  }

  /* Remember the selected PID so the selection follows the process
     across re-sorts instead of sticking to a rank. */
  pid_t selected_pid = -1;
  if (selected_index >= 0 && selected_index < (int)process_count) {
    selected_pid = processes[selected_index].pid;
  }

  for (size_t i = 0; i < count; i++) {
    processes[i] = snapshots[i];
  }

  process_count = count;

  if (sort_mode == SORT_BY_CPU) {
    qsort(processes, process_count,
          sizeof(ResourceSnapshot), compare_cpu
    );
  } else {
    qsort(processes, process_count, sizeof(ResourceSnapshot), compare_memory);
  }

  for (size_t i = 0; i < process_count; i++) {
    if (processes[i].pid == selected_pid) {
      selected_index = (int)i;
      break;
    }
  }

  clamp_view(visible_rows);
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

/* ---- Bounds-checked drawing: nothing is written outside the screen and
   strings are truncated at the right edge instead of wrapping. ---- */

__attribute__((format(printf, 3, 4)))
static void put(int row, int col, const char *fmt, ...) {
  if (row < 0 || row >= LINES || col < 0 || col >= COLS) {
    return;
  }

  char buf[256];
  va_list ap;
  va_start(ap, fmt);
  vsnprintf(buf, sizeof(buf), fmt, ap);
  va_end(ap);

  mvaddnstr(row, col, buf, COLS - col);
}

static void put_ch(int row, int col, chtype ch) {
  if (row >= 0 && row < LINES && col >= 0 && col < COLS) {
    mvaddch(row, col, ch);
  }
}

static void hline_at(int row, int col, int width) {
  if (row >= 0 && row < LINES && col >= 0 && col < COLS && width > 0) {
    mvhline(row, col, ACS_HLINE, width);
  }
}

/* ---- Layout ----
   gap = 1 keeps the original spacious positions; gap = 0 drops blank
   separator rows. chart_height = 0 hides the CPU history chart. */
typedef struct {
  int gap;
  int chart_height;
} Layout;

static const Layout LAYOUTS[] = {
  {1, 7},   /* large: original layout */
  {0, 5},   /* medium: no spacing, shorter chart */
  {0, 0},   /* small: metrics + process table only */
};

/* Must mirror the row arithmetic of the render_* functions below. */
static int layout_data_row(Layout l) {
  int y = 2 + l.gap;                    /* header */
  y += l.gap ? 5 : 2;                   /* summary */
  y += 6 + 2 * l.gap;                   /* system metrics */
  if (l.chart_height > 0) {
    y += l.chart_height + 2 + l.gap;    /* cpu history */
  }
  return y + 3 + 2 * l.gap;             /* table title + header + rule */
}

static int layout_min_lines(Layout l) {
  return layout_data_row(l) + MIN_TABLE_ROWS + FOOTER_ROWS;
}

static int pick_layout(int lines, Layout *out) {
  for (size_t i = 0; i < sizeof(LAYOUTS) / sizeof(LAYOUTS[0]); i++) {
    if (lines >= layout_min_lines(LAYOUTS[i])) {
      *out = LAYOUTS[i];
      return 1;
    }
  }
  return 0;
}

/* Each render_* takes its top row and returns the next free row. */

static int render_header(int gap) {
    attron(COLOR_PAIR(1) | A_BOLD);
    put(0, 2, "MINI-SANDBOX MONITOR");
    attroff(COLOR_PAIR(1) | A_BOLD);

    attron(COLOR_PAIR(2));
    put(0, COLS - 12, "LIVE");
    attroff(COLOR_PAIR(2));

    hline_at(1, 0, COLS);
    return 2 + gap;
}

static int render_summary(int y, int gap) {
    unsigned int running = 0;
    unsigned int total_threads = 0;

    for (size_t i = 0; i < process_count; i++) {
      if (processes[i].state == 'R') {
            running++;
      }

       total_threads += processes[i].threads;
    }

    if (gap) {
        put(y, 2, "SYSTEM SUMMARY");
        y += 2;
    }

    put(y, 5, "Processes: %zu", process_count);
    put(y, 30, "Running: %u", running);
    put(y, 55, "Threads: %u", total_threads);

    hline_at(y + 1 + gap, 0, COLS);
    return y + 2 + gap;
}

static int render_system_summary(int y, int gap) {
    put(y, 2, "SYSTEM METRICS");

    int r = y + 1 + gap;

    if (!has_system_metrics) {
        put(r, 5, "Waiting for system metrics...");
    } else {
        put(r, 5, "CPU Usage: %.2f%% | Logical CPUs: %u",
            system_metrics.cpu_usage, system_metrics.cpu_count);
        put(r + 1, 5, "Memory Usage: %.2f%% | Used: %lu KB | Available: %lu KB",
            system_metrics.memory_usage, system_metrics.memory_used_kb,
            system_metrics.memory_available_kb);
        put(r + 2, 5, "Load Average: %.2f / %.2f / %.2f",
            system_metrics.load_average_1, system_metrics.load_average_5,
            system_metrics.load_average_15);
        put(r + 3, 5, "Uptime: %lu seconds", system_metrics.uptime_seconds);
    }

    hline_at(r + 4 + gap, 0, COLS);
    return r + 5 + gap;
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
    put_ch(row, col + i, i < filled ? '#' : '-');
  }
}

static int render_cpu_history(int y, int gap, int height) {
  const int chart_top = y + 1 + gap;
  const int chart_bottom = chart_top + height - 1;
  const int chart_mid = (chart_top + chart_bottom) / 2;
  const int chart_left = 10;
  const int next_row = chart_bottom + 2;

  hline_at(chart_bottom + 1, 0, COLS);

  put(y, 2, "CPU HISTORY - PID: %d (Last 60 Samples)", cpu_history_pid);

  int chart_width = COLS - chart_left - 2;

  if (chart_width > CPU_HISTORY_LENGTH) {
    chart_width = CPU_HISTORY_LENGTH;
  }

  if (chart_width <= 0 || cpu_history_count == 0) {
    return next_row;
  }

  put(chart_top, 2, "100%%");
  put(chart_mid, 2, "50%%");
  put(chart_bottom, 2, "0%%");

  for (int x = chart_left; x < chart_left + chart_width; x++) {
    put_ch(chart_top, x, ACS_HLINE);
    put_ch(chart_mid, x, ACS_HLINE);
    put_ch(chart_bottom, x, ACS_HLINE);
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

    int py = chart_bottom - (int)((value / 100.0) * (chart_bottom - chart_top));
    int px = chart_left + (int)i;

    if (previous_y >= 0) {
      if (py == previous_y) {
        put_ch(py, px, ACS_HLINE);
      } else if (py < previous_y) {
        put_ch(py, px, '/');
      } else {
        put_ch(py, px, '\\');
      }
    }

    put_ch(py, px, '*');

    previous_y = py;
  }

  return next_row;
}

static void render_process_table(int y, int gap) {
    const int title_row = y + gap;
    const int header_row = title_row + 1 + gap;
    const int first_data_row = header_row + 2;

    const int cpu_bar_width = 20;
    const int memory_bar_width = 20;

    visible_rows = LINES - FOOTER_ROWS - first_data_row;
    if (visible_rows < 1) {
        visible_rows = 1;
    }
    clamp_view(visible_rows);

    unsigned long max_rss = 0;

    for (size_t i = 0; i < process_count; i++) {
      if (processes[i].vm_rss_kb > max_rss) {
        max_rss = processes[i].vm_rss_kb;
      }
    }

    int last_shown = scroll_offset + visible_rows;
    if (last_shown > (int)process_count) {
        last_shown = (int)process_count;
    }

    if (process_count > 0) {
        put(title_row, 2, "PROCESS MONITOR  [%d-%d of %zu]",
            scroll_offset + 1, last_shown, process_count);
    } else {
        put(title_row, 2, "PROCESS MONITOR");
    }

    put(header_row, 2, "PID");
    put(header_row, 11, "STATE");
    put(header_row, 20, "CPU%%");
    put(header_row, 30, "CPU USAGE");
    put(header_row, 54, "RSS(KB)");
    put(header_row, 68, "MEMORY USAGE");
    put(header_row, 93, "THREADS");

    hline_at(header_row + 1, 2, COLS - 4);

    if (process_count == 0) {
        put(first_data_row, 2, "No processes");
        return;
    }

    for (int visible_row = 0; visible_row < visible_rows; visible_row++) {
      int process_index = scroll_offset + visible_row;

      if (process_index < 0 || process_index >= (int)process_count) {
        break;
      }

      const ResourceSnapshot *p = &processes[process_index];
      int row = first_data_row + visible_row;
      int selected = process_index == selected_index;

      if (selected) {
        attron(A_REVERSE);
      }

      put(row, 2, "%-8d", p->pid);
      put(row, 11, "%-8c", p->state);
      put(row, 20, "%7.2f%%", p->cpu_usage);
      render_bar(row, 30, p->cpu_usage, 100.0, cpu_bar_width);
      put(row, 54, "%-10lu", p->vm_rss_kb);
      render_bar(row, 68, (double)p->vm_rss_kb, (double)max_rss,
                 memory_bar_width);
      put(row, 93, "%-8u", p->threads);

      if (selected) {
        attroff(A_REVERSE);
      }
    }
}

static void render_footer(void) {
  hline_at(LINES - 2, 0, COLS);

  const char *sort_label =
        sort_mode == SORT_BY_CPU ? "CPU DESC" : "RSS DESC";

  put(LINES - 1, 2,
      "Sort: %s | UP/DOWN PgUp/PgDn Home/End: Select | c: CPU Sort | "
      "m: Memory Sort | q: Quit",
      sort_label);
}

static void render_too_small(void) {
  put(0, 0, "Terminal too small: %dx%d", COLS, LINES);
  put(1, 0, "Need at least %dx%d. q: Quit",
      MIN_COLS, layout_min_lines(LAYOUTS[sizeof(LAYOUTS) / sizeof(LAYOUTS[0]) - 1]));
}

void dashboard_render(void) {
    erase();

    Layout layout;

    if (COLS < MIN_COLS || !pick_layout(LINES, &layout)) {
        render_too_small();
        refresh();
        return;
    }

    int y = render_header(layout.gap);
    y = render_summary(y, layout.gap);
    y = render_system_summary(y, layout.gap);

    if (layout.chart_height > 0) {
        y = render_cpu_history(y, layout.gap, layout.chart_height);
    }

    render_process_table(y, layout.gap);
    render_footer();

    refresh();
}

void dashboard_handle_input(int key) {
  switch (key) {
    case KEY_UP:    selected_index--; break;
    case KEY_DOWN:  selected_index++; break;
    case KEY_PPAGE: selected_index -= visible_rows; break;
    case KEY_NPAGE: selected_index += visible_rows; break;
    case KEY_HOME:  selected_index = 0; break;
    case KEY_END:   selected_index = (int)process_count - 1; break;

    case 'c':
    case 'C':
      sort_mode = SORT_BY_CPU;
      qsort(processes, process_count, sizeof(ResourceSnapshot), compare_cpu);
      selected_index = 0;
      scroll_offset = 0;
      break;

    case 'm':
    case 'M':
      sort_mode = SORT_BY_MEMORY;
      qsort(processes, process_count, sizeof(ResourceSnapshot), compare_memory);
      selected_index = 0;
      scroll_offset = 0;
      break;

    default:
      /* KEY_RESIZE needs nothing here: ncurses has already updated
         LINES/COLS and the next render re-picks the layout. */
      break;
  }

  clamp_view(visible_rows);
}
