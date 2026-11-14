/* Self-check for dashboard scroll/layout logic (no terminal needed).
   Build: gcc -std=c11 -Iinclude tests/dashboard_scroll_test.c -o dashboard_scroll_test -lncurses */
#include <assert.h>
#include <stdio.h>

#include "../src/dashboard.c"

static void set_count(size_t n) {
  process_count = n;
}

int main(void) {
  /* Scrolling down past the viewport keeps selection on the last row. */
  set_count(100);
  selected_index = 0; scroll_offset = 0;
  for (int i = 0; i < 12; i++) { selected_index++; clamp_view(10); }
  assert(selected_index == 12 && scroll_offset == 3);

  /* Scrolling back up pulls the viewport with the selection. */
  for (int i = 0; i < 10; i++) { selected_index--; clamp_view(10); }
  assert(selected_index == 2 && scroll_offset == 2);

  /* END / past-the-end clamps to the last process, viewport at bottom. */
  selected_index = 500; clamp_view(10);
  assert(selected_index == 99 && scroll_offset == 90);

  /* Viewport grows (resize): offset shrinks so no empty rows trail. */
  clamp_view(50);
  assert(selected_index == 99 && scroll_offset == 50);

  /* List shrinks below viewport: offset 0, selection clamped. */
  set_count(4); clamp_view(10);
  assert(selected_index == 3 && scroll_offset == 0);

  /* Empty list. */
  set_count(0); selected_index = 7; scroll_offset = 5; clamp_view(10);
  assert(selected_index == 0 && scroll_offset == 0);

  /* Large layout reproduces the original first data row (31). */
  assert(layout_data_row(LAYOUTS[0]) == 31);

  /* Layout tiers by height. */
  Layout l;
  assert(pick_layout(50, &l) && l.gap == 1 && l.chart_height == 7);
  assert(pick_layout(30, &l) && l.gap == 0 && l.chart_height == 5);
  assert(pick_layout(20, &l) && l.chart_height == 0);
  assert(!pick_layout(10, &l));

  puts("dashboard_scroll_test: OK");
  return 0;
}
