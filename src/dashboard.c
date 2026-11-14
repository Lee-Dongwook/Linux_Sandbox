#include <ncurses.h>

#include "dashboard.h"

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
    mvprintw(3, 2, "SYSTEM SUMMARY");

    mvprintw(5, 4, "Processes: %d", 2);
    mvprintw(5, 30, "Running: %d", 1);
    mvprintw(5, 55, "Threads: %d", 2);

    mvhline(7, 0, ACS_HLINE, COLS);
}

static void render_process_table(void) {
    mvprintw(9, 2, "PROCESS MONITOR");

    mvprintw(
        11, 2,
        "%-8s %-8s %-8s %-10s %-12s %-12s %-8s",
        "PID", "PPID", "STATE", "CPU%", "RSS", "VIRT", "THREADS"
    );

    mvhline(12, 2, ACS_HLINE, COLS - 4);

    mvprintw(
        14, 2,
        "%-8d %-8d %-8c %-10.2f %-12s %-12s %-8d",
        408, 1, 'R', 99.51, "1500 KB", "2384 KB", 1
    );

    mvprintw(
        15, 2,
        "%-8d %-8d %-8c %-10.2f %-12s %-12s %-8d",
        1, 0, 'S', 0.00, "3648 KB", "4100 KB", 1
    );
}

static void render_footer(void) {
    mvhline(LINES - 2, 0, ACS_HLINE, COLS);

    mvprintw(
        LINES - 1, 2,
        "Refresh: 1s | Sort: CPU DESC | q: Quit"
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
