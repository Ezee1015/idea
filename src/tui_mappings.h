#ifndef MAPPINGS

#include <stdbool.h>

#define ESCAPE_KEY 27
#define BACKSPACE_KEY 127
#define ENTER_KEY 10

#define STRINGIFY(...) (char[]){__VA_ARGS__, '\0'}

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
