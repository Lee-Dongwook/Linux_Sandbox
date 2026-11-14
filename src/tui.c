#include <ncurses.h>

#include "tui.h"
#include "dashboard.h"

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

    int running = 1;

    while (running) {
        dashboard_render();

        int key = getch();

        if (key == 'q' || key == 'Q') {
            running = 0;
        }
    }

    endwin();

    return 0;
}
