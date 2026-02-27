#ifndef MAPPINGS

#include <stdbool.h>
#include <ctype.h>

#define ESCAPE_KEY 27
#define BACKSPACE_KEY 127
#define ENTER_KEY 10

#define STRINGIFY(...) (char[]){__VA_ARGS__, '\0'}
//////////////////////////////////////////////////////
///////////////////// INPUT MAPS /////////////////////
//////////////////////////////////////////////////////

#define C_INPUT_IS_VALID_INSERT_CHAR(c) (isalnum(c) || c == ' ' || c == '_' || c == '\\' || c == '.' || c == '/' || c == '!')
#define CMD_INPUT_MODE_KEY '`'

typedef enum {
  COMMAND_INSERT,
  COMMAND_NORMAL,
} Command_input_mode;
extern Command_input_mode command_input_mode;

typedef struct {
  char *keys;
  char *description;
  void (*action)(int *input_cursor, int *input_length, int screen_y, int *screen_x);
} Command_map;

void c_map_left(int *input_cursor, int *input_length, int screen_y, int *screen_x);
void c_map_right(int *input_cursor, int *input_length, int screen_y, int *screen_x);
void c_map_quit(int *input_cursor, int *input_length, int screen_y, int *screen_x);
void c_map_insert(int *input_cursor, int *input_length, int screen_y, int *screen_x);
void c_map_append(int *input_cursor, int *input_length, int screen_y, int *screen_x);
void c_map_backward_word(int *input_cursor, int *input_length, int screen_y, int *screen_x);
void c_map_forward_word(int *input_cursor, int *input_length, int screen_y, int *screen_x);
void c_map_forward_word_end(int *input_cursor, int *input_length, int screen_y, int *screen_x);
void c_map_start(int *input_cursor, int *input_length, int screen_y, int *screen_x);
void c_map_end(int *input_cursor, int *input_length, int screen_y, int *screen_x);
void c_map_insert_start(int *input_cursor, int *input_length, int screen_y, int *screen_x);
void c_map_append_end(int *input_cursor, int *input_length, int screen_y, int *screen_x);
void c_map_delete_inner_word(int *input_cursor, int *input_length, int screen_y, int *screen_x);
void c_map_delete_around_word(int *input_cursor, int *input_length, int screen_y, int *screen_x);
void c_map_change_inner_word(int *input_cursor, int *input_length, int screen_y, int *screen_x);
void c_map_change_around_word(int *input_cursor, int *input_length, int screen_y, int *screen_x);
void c_map_remove_char(int *input_cursor, int *input_length, int screen_y, int *screen_x);
void c_map_next_char(int *input_cursor, int *input_length, int screen_y, int *screen_x);
void c_map_previous_char(int *input_cursor, int *input_length, int screen_y, int *screen_x);

extern Command_map c_maps[];
extern unsigned int c_maps_count;

////////////////////////////////////////////////////////
////////////////// Normal-Visual MAPS //////////////////
////////////////////////////////////////////////////////

#define APPLY_MULTIPLIER(command) do {                                                            \
  if (tui_st.command_multiplier == 0) {                                                           \
    command;                                                                                      \
  } else {                                                                                        \
    for (unsigned int i=0; i<tui_st.command_multiplier && i < list_size(todo_list); i++) command; \
    tui_st.command_multiplier = 0;                                                                \
  }                                                                                               \
} while (0)

typedef struct {
  char *keys;
  char *description;
  void (*action)();
} Normal_visual_map;

void nv_map_toggle();
void nv_map_cursor_down();
void nv_map_cursor_up();
void nv_map_move_to_top();
void nv_map_move_to_bottom();
void nv_map_save();
void nv_map_save_and_exit();
void nv_map_force_quit();
void nv_map_quit();
void nv_map_toggle_visual();
void nv_map_move_down();
void nv_map_move_up();
void nv_map_unselect();
void nv_map_command();
void nv_map_clear_command_multiplier();
void nv_map_delete();
void nv_map_add_below_cursor();
void nv_map_add_above_cursor();
void nv_map_edit();

extern Normal_visual_map nv_maps[];
extern unsigned int nv_maps_count;

#endif // MAPPINGS
