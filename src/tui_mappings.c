#include "tui_mappings.h"
#include "tui.h"

#define UNUSED(var) (void) var;

//////////////////////////////////////////////////////
///////////////////// INPUT MAPS /////////////////////
//////////////////////////////////////////////////////

void c_map_left(int *input_cursor, int *input_length, int screen_y, int *screen_x) {
  UNUSED(input_length);

  if (!*input_cursor) return;
  move(screen_y, --(*screen_x)); (*input_cursor)--;
}

void c_map_right(int *input_cursor, int *input_length, int screen_y, int *screen_x) {
  if (*input_cursor >= *input_length) return;
  move(screen_y, ++(*screen_x));
  (*input_cursor)++;
}

void c_map_quit(int *input_cursor, int *input_length, int screen_y, int *screen_x) {
  UNUSED(input_cursor);
  UNUSED(input_length);
  UNUSED(screen_y);
  UNUSED(screen_x);

  tui_st.mode = MODE_NORMAL;
}

void c_map_insert(int *input_cursor, int *input_length, int screen_y, int *screen_x) {
  UNUSED(input_cursor);
  UNUSED(input_length);
  UNUSED(screen_y);
  UNUSED(screen_x);

  tui_st.command_input_mode = COMMAND_INPUT_INSERT;
}

void c_map_append(int *input_cursor, int *input_length, int screen_y, int *screen_x) {
  if (*input_cursor == *input_length) return;
  (*input_cursor)++;
  move(screen_y, ++(*screen_x));
  tui_st.command_input_mode = COMMAND_INPUT_INSERT;
}

void c_map_backward_word(int *input_cursor, int *input_length, int screen_y, int *screen_x) {
  UNUSED(input_length);

  unsigned int start = *input_cursor;
  if (*input_cursor > 0) (*input_cursor)--; // To skip the space at the left of the cursor when pressing multiple times 'b'
  if (command_input_move_to_previous_character(input_cursor, STRINGIFY(' '))) (*input_cursor)++;
  unsigned int delta = start - *input_cursor;
  (*screen_x) -= delta;
  move(screen_y, *screen_x);
}

void c_map_forward_word(int *input_cursor, int *input_length, int screen_y, int *screen_x) {
  unsigned int start = *input_cursor;
  if (command_input_move_to_next_character(input_cursor, *input_length, STRINGIFY(' '))) (*input_cursor)++;
  unsigned int delta = *input_cursor - start;
  *screen_x += delta;
  move(screen_y, *screen_x);
}

void c_map_forward_word_end(int *input_cursor, int *input_length, int screen_y, int *screen_x) {
  unsigned int start = *input_cursor;
  if (*input_cursor == *input_length-1) {
    *input_cursor = *input_length;
  } else {
    (*input_cursor)++; // To skip the space at the right of the cursor when pressing multiple times 'e'
    command_input_move_to_next_character(input_cursor, *input_length, STRINGIFY(' '));
    (*input_cursor)--; // Go to the last letter of the word
  }
  unsigned int delta = *input_cursor - start;
  *screen_x += delta;
  move(screen_y, *screen_x);
}

void c_map_start(int *input_cursor, int *input_length, int screen_y, int *screen_x) {
  UNUSED(input_length);

  *screen_x -= *input_cursor;
  move(screen_y, *screen_x);
  *input_cursor = 0;
}

void c_map_end(int *input_cursor, int *input_length, int screen_y, int *screen_x) {
  *screen_x += *input_length - *input_cursor;
  move(screen_y, *screen_x);
  *input_cursor = *input_length;
}

void c_map_insert_start(int *input_cursor, int *input_length, int screen_y, int *screen_x) {
  c_map_start(input_cursor, input_length, screen_y, screen_x);
  tui_st.command_input_mode = COMMAND_INPUT_INSERT;
}

void c_map_append_end(int *input_cursor, int *input_length, int screen_y, int *screen_x) {
  c_map_end(input_cursor, input_length, screen_y, screen_x);
  tui_st.command_input_mode = COMMAND_INPUT_INSERT;
}

void c_map_delete_inner_word(int *input_cursor, int *input_length, int screen_y, int *screen_x) {
  command_input_remove_word(input_cursor, input_length, screen_x, screen_y, false);
}

void c_map_delete_around_word(int *input_cursor, int *input_length, int screen_y, int *screen_x) {
  command_input_remove_word(input_cursor, input_length, screen_x, screen_y, true);
}

void c_map_change_inner_word(int *input_cursor, int *input_length, int screen_y, int *screen_x) {
  c_map_delete_inner_word(input_cursor, input_length, screen_y, screen_x);
  tui_st.command_input_mode = COMMAND_INPUT_INSERT;
}

void c_map_change_around_word(int *input_cursor, int *input_length, int screen_y, int *screen_x) {
  c_map_delete_around_word(input_cursor, input_length, screen_y, screen_x);
  tui_st.command_input_mode = COMMAND_INPUT_INSERT;
}

void c_map_remove_char(int *input_cursor, int *input_length, int screen_y, int *screen_x) {
  if (*input_length == 0 || *input_cursor == *input_length) return;

  for (int z = *input_cursor; z < *input_length; z++) {
    input[z] = input[z+1];
    addch((z == *input_length-1) ? ' ' : input[z]);
  }
  (*input_length)--;
  if (*input_cursor == *input_length && *input_cursor != 0) {
    (*input_cursor)--;
    (*screen_x)--;
  }
  move(screen_y, *screen_x);
}

void c_map_next_char(int *input_cursor, int *input_length, int screen_y, int *screen_x) {
  char c = getch();
  (*screen_x)++;

  // Backspace and Escape keys when pressed they print 2 characters
  unsigned int chars_to_clear = (c == BACKSPACE_KEY || c == ESCAPE_KEY) ? 2 : 1;
  command_input_refresh_characters(chars_to_clear, screen_y, screen_x, input_cursor, *input_length);

  if (!C_INPUT_IS_VALID_INSERT_CHAR(c)) return;

  int new_cursor = *input_cursor;
  do {
    if (!command_input_move_to_next_character(&new_cursor, *input_length, STRINGIFY(c))) return;
    if (tui_st.command_multiplier > 0) tui_st.command_multiplier--;
  } while(tui_st.command_multiplier > 0);

  unsigned int delta = new_cursor - *input_cursor;
  *screen_x += delta;
  move(screen_y, *screen_x);
  *input_cursor = new_cursor;
}

void c_map_previous_char(int *input_cursor, int *input_length, int screen_y, int *screen_x) {
  char c = getch();
  (*screen_x)++;

  // Backspace and Escape keys when pressed they print 2 characters
  unsigned int chars_to_clear = (c == BACKSPACE_KEY || c == ESCAPE_KEY) ? 2 : 1;
  command_input_refresh_characters(chars_to_clear, screen_y, screen_x, input_cursor, *input_length);

  if (!C_INPUT_IS_VALID_INSERT_CHAR(c)) return;

  int new_cursor = *input_cursor;
  do {
    if (!command_input_move_to_previous_character(&new_cursor, STRINGIFY(c))) return;
    if (tui_st.command_multiplier > 0) tui_st.command_multiplier--;
  } while(tui_st.command_multiplier > 0);

  unsigned int delta = new_cursor - *input_cursor;
  *screen_x += delta;
  move(screen_y, *screen_x);
  *input_cursor = new_cursor;
}

Command_map c_maps[] = {
  { STRINGIFY(CMD_INPUT_MODE_KEY), "INSERT-NORMAL: Switch the input between insert and normal mode", c_map_insert },
  { "q"  , "NORMAL: Quit input"                            , c_map_quit },
  { "i"  , "NORMAL: Insert"                                , c_map_insert },
  { "a"  , "NORMAL: Append"                                , c_map_append },
  { "h"  , "NORMAL: Move left"                             , c_map_left },
  { "l"  , "NORMAL: Move right"                            , c_map_right },
  { "b"  , "NORMAL: Move backward a word"                  , c_map_backward_word },
  { "w"  , "NORMAL: Move forward a word"                   , c_map_forward_word },
  { "e"  , "NORMAL: Move forward to the end of word"       , c_map_forward_word_end },
  { "x"  , "NORMAL: Remove the character under the cursor" , c_map_remove_char },
  { "gh" , "NORMAL: Go to the first character of the input", c_map_start },
  { "gl" , "NORMAL: Go to the last character of the input" , c_map_end },
  { "I"  , "NORMAL: Insert text at the start of the input" , c_map_insert_start },
  { "A"  , "NORMAL: Append text at the end of the input"   , c_map_append_end },
  { "diw", "NORMAL: Delete inner word"                     , c_map_delete_inner_word },
  { "daw", "NORMAL: Delete a word"                         , c_map_delete_around_word },
  { "ciw", "NORMAL: Change inner word"                     , c_map_change_inner_word },
  { "caw", "NORMAL: Change a word"                         , c_map_change_around_word },
  { "f"  , "NORMAL: Jump to the next given character"      , c_map_next_char },
  { "F"  , "NORMAL: Jump to the previous given character"  , c_map_previous_char },
};

unsigned int c_maps_count = sizeof(c_maps) / sizeof(Command_map);

////////////////////////////////////////////////////////
////////////////// Normal-Visual MAPS //////////////////
////////////////////////////////////////////////////////

#define APPLY_MULTIPLIER(command) do {                                                                \
  if (tui_st.command_multiplier == 0) {                                                               \
    command;                                                                                          \
  } else {                                                                                            \
    for (unsigned int _i=0; _i<tui_st.command_multiplier && _i < list_size(todo_list); _i++) command; \
  }                                                                                                   \
} while (0)

void nv_map_toggle() {
  APPLY_MULTIPLIER({
    toggle_select_item();
    next_position();
  });
}

void nv_map_cursor_down() {
  APPLY_MULTIPLIER({
    switch (tui_st.mode) {
      case MODE_NORMAL:  next_position();       break;
      case MODE_VISUAL:  visual_move_cursor(1); break;
      case MODE_COMMAND: break; /* unreachable */
    }
  });
}

void nv_map_cursor_up() {
  APPLY_MULTIPLIER({
    switch (tui_st.mode) {
      case MODE_NORMAL: previous_position();    break;
      case MODE_VISUAL: visual_move_cursor(-1); break;
      case MODE_COMMAND: break; /* unreachable */
    }
  });
}

void nv_map_move_to_top() {
  switch (tui_st.mode) {
    case MODE_NORMAL:
      if (!list_is_empty(tui_st.selected)) {
        while (move_selected(-1));
      }
      while (previous_position());
      break;
    case MODE_VISUAL: while (tui_st.current_pos) visual_move_cursor(-1); break;
    case MODE_COMMAND: break; /* unreachable */
  }
}
void nv_map_move_to_bottom() {
  if (list_is_empty(todo_list)) return;

  switch (tui_st.mode) {
    case MODE_NORMAL:
      if (!list_is_empty(tui_st.selected)) {
        while (move_selected(1));
      }
      while (next_position());
      break;
    case MODE_VISUAL: while (tui_st.current_pos < list_size(todo_list)-1) visual_move_cursor(1); break;
    case MODE_COMMAND: break; /* unreachable  */
  }
}

void nv_map_save() { action_save(NULL); }
void nv_map_save_and_exit() { action_save_and_quit(NULL); }
void nv_map_force_quit() { action_force_quit(NULL); }
void nv_map_quit() { action_quit(NULL); }

void nv_map_toggle_visual() {
  switch (tui_st.mode) {
    case MODE_NORMAL:
      tui_st.visual_start_pos = tui_st.current_pos;
      tui_st.mode = MODE_VISUAL;
      tui_st.visual_mode = (is_current_item_selected())
                           ? VISUAL_UNSELECT
                           : VISUAL_SELECT;

      APPLY_MULTIPLIER({
        switch (tui_st.visual_mode) {
          case VISUAL_SELECT:   select_current_item();   break;
          case VISUAL_UNSELECT: unselect_current_item(); break;
        }
        next_position();
      });

      if (tui_st.current_pos != list_size(todo_list)-1) previous_position();
      break;

    case MODE_VISUAL:
      tui_st.mode = MODE_NORMAL;
      break;

    case MODE_COMMAND: break; /* unreachable */
  }
}

void nv_map_move_down() {
  APPLY_MULTIPLIER({
      if (move_selected(1)) {
        next_position();
        if (tui_st.mode == MODE_VISUAL) tui_st.visual_start_pos++;
        todo_list_modified = true;
      }
  });
}

void nv_map_move_up() {
  APPLY_MULTIPLIER({
      if (move_selected(-1)) {
        previous_position();
        if (tui_st.mode == MODE_VISUAL) tui_st.visual_start_pos--;
        todo_list_modified = true;
      }
  });
}
void nv_map_unselect() {
  if (tui_st.mode == MODE_NORMAL) list_destroy(&tui_st.selected, NULL);
}

void nv_map_command() {
  if (tui_st.mode == MODE_NORMAL) tui_st.mode = MODE_COMMAND;
  curs_set(1);
  tui_st.command_input_mode = COMMAND_INPUT_INSERT;
}

void nv_map_delete() {
  if (tui_st.mode == MODE_NORMAL) {
    /* if (list_is_empty(tui_st.selected)) select_current_item(); */

    if (delete_selected()) todo_list_modified = true;
  }
}

void nv_map_add_below_cursor() {
  unsigned int pos = (list_is_empty(todo_list)) ? 1 : tui_st.current_pos + 1 /* 0-based to 1-based) */ + 1 /* next pos */;
  populate_command_input("add_at %d ", pos);
}

void nv_map_add_above_cursor() {
  populate_command_input("add_at %d ", tui_st.current_pos + 1 /* 0-based to 1-based) */);
}

void nv_map_edit() {
  const char *todo_name = ((Todo *)list_get(todo_list, tui_st.current_pos))->name;
  populate_command_input("edit %d %s", tui_st.current_pos + 1 /* 0-based to 1-based) */, todo_name);
}

Normal_visual_map nv_maps[] = {
  { " "                   , "Toggle select"                                       , nv_map_toggle },
  { "j"                   , "Move the cursor down"                                , nv_map_cursor_down },
  { "k"                   , "Move the cursor up"                                  , nv_map_cursor_up },
  { "gg"                  , "Move the cursor and the selected items to the top"   , nv_map_move_to_top },
  { "G"                   , "Move the cursor and the selected items to the bottom", nv_map_move_to_bottom },
  { "s"                   , "Save the changes to disk (same as :w)"               , nv_map_save },
  { "W"                   , "Save and Exit idea (same as :wq)"                    , nv_map_save_and_exit },
  { "Q"                   , "Exit idea without saving (same as :q!)"              , nv_map_force_quit },
  { "q"                   , "Confirm to Save and Exit idea (same as :q)"          , nv_map_quit },
  { "V"                   , "Enter or Exit visual mode"                           , nv_map_toggle_visual },
  { "J"                   , "Move the selected ToDos down"                        , nv_map_move_down },
  { "K"                   , "Move the selected ToDos up"                          , nv_map_move_up },
  { "us"                  , "Unselect all the selected items"                     , nv_map_unselect },
  { ":"                   , "Enter command mode"                                  , nv_map_command },
  { "d"                   , "Delete the selected ToDos"                           , nv_map_delete },
  { "o"                   , "Create a new ToDo below the cursor"                  , nv_map_add_below_cursor },
  { "O"                   , "Create a new ToDo above the cursor"                  , nv_map_add_above_cursor },
  { "a"                   , "Change the name of the current ToDo"                 , nv_map_edit },
};

unsigned int nv_maps_count = sizeof(nv_maps) / sizeof(Normal_visual_map);
