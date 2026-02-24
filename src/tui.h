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
void tui_print_backtrace();

void update_area_x_axis();
void update_area_y_axis();

// Command mode
Help_result show_functionality_message(const char *source, const unsigned source_index, const unsigned int source_count, const Functionality *functionality, const unsigned int functionality_count, bool are_commands, bool from_the_end, unsigned int *max_functionality_per_page);
bool parse_command();

// Actions for command mode
bool action_quit(Input *input);
bool action_save(Input *input);
bool action_save_and_quit(Input *input);
bool action_help(Input *input);
bool action_tui_version(Input *input);
bool action_add_at_todo_tui(Input *input);

extern Functionality tui_functionality[];
extern unsigned int tui_functionality_count;

// Vi-like input in the TUI
bool vi_input_move_to_previous_space(int *cursor);
bool vi_input_move_to_next_space(int *cursor, int length);
void vi_input_remove_word(int *cursor, int *length, int *cursor_x, int cursor_y, bool around);
void vi_input_refresh_characters(int chars_to_clear, int cursor_y, int *cursor_x, int *input_cursor, int input_len, char *input);

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
void populate_input(const char *fmt, ...) __attribute__((format(printf, 1, 2)));

#endif // TUI_H
