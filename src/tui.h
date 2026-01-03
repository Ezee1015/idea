#ifndef TUI_H
#define TUI_H

#include <ncurses.h>
#include "utils/list.h"

typedef struct {
  unsigned int width;
  unsigned int height;
} Size;

typedef struct {
  unsigned int x;
  unsigned int y;
} Point;

typedef struct {
  enum {
    MODE_NORMAL,
    MODE_VISUAL,
    MODE_COMMAND,
  } mode;

  List selected;

  unsigned int current_pos;

  unsigned int command_multiplier;

  enum {
    VISUAL_SELECT,
    VISUAL_UNSELECT
  } visual_mode;

  unsigned int visual_start_pos;

} Tui_state;

// Window drawing
void message(char *title, char *msg);
void draw_window(void);
bool window_app(void);

void update_area_x_axis();
void update_area_y_axis();

// Command mode
bool parse_command(WINDOW *win, bool *exit_loop);

// Normal-Visual mode
bool is_current_item_selected();
bool select_current_item();
bool unselect_current_item();
void toggle_select_item();
bool next_position();
bool previous_position();
bool move_selected(int direction);
void visual_move_cursor(int direction);
void parse_normal(bool *exit_loop);

#endif // TUI_H
