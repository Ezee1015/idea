#ifndef TUI_H
#define TUI_H

#include <ncurses.h>
#include "utils/list.h"
#include "todo_list.h"

typedef struct {
  unsigned int width;
  unsigned int height;
} Size;

typedef struct {
  unsigned int x;
  unsigned int y;
} Point;

typedef struct {
  bool exit_loop;
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

typedef enum {
  CONFIRM_NORMAL,
  CONFIRM_DEFAULT_YES,
  CONFIRM_DEFAULT_NO,
} Confirm_type;

typedef enum {
  HELP_RETURN_NEXT,
  HELP_RETURN_PREV,
  HELP_RETURN_QUIT,
} Help_result;

// Window drawing
void draw_rect(int y1, int x1, int y2, int x2);
char message(char *title, char *msg);
bool confirm(char *msg, Confirm_type type);
void draw_window(void);
bool window_app(void);

void update_area_x_axis();
void update_area_y_axis();

// Command mode
Help_result show_functionality_message(const char *source, const Functionality *functionality, const unsigned int functionality_count, bool from_the_end, unsigned int *max_functionality_per_page);
bool parse_command(WINDOW *win);

// Actions for command mode
Action_return action_quit(Input *input);
Action_return action_save_and_quit(Input *input);
Action_return action_help(Input *input);
Action_return action_tui_version(Input *input);

extern Functionality tui_functionality[];
extern unsigned int tui_functionality_count;


// Normal-Visual mode
bool is_current_item_selected();
bool select_current_item();
bool unselect_current_item();
void toggle_select_item();
bool next_position();
bool previous_position();
bool move_selected(int direction);
bool delete_selected();
void visual_move_cursor(int direction);
void parse_normal();

#endif // TUI_H
