#include "tui_mappings.h"
#include "tui.h"

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
  tui_st.command_multiplier = 0;
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
  tui_st.command_multiplier = 0;
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
      tui_st.command_multiplier = 0;
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
  tui_st.command_multiplier = 0;
}

void nv_map_command() {
  if (tui_st.mode == MODE_NORMAL) tui_st.mode = MODE_COMMAND;
  tui_st.command_multiplier = 0;
  curs_set(1);
}

void nv_map_clear_command_multiplier() {
  if (tui_st.command_multiplier) tui_st.command_multiplier = 0;
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
  tui_st.command_multiplier = 0;
}

void nv_map_add_above_cursor() {
  populate_command_input("add_at %d ", tui_st.current_pos + 1 /* 0-based to 1-based) */);
  tui_st.command_multiplier = 0;
}

void nv_map_edit() {
  const char *todo_name = ((Todo *)list_get(todo_list, tui_st.current_pos))->name;
  populate_command_input("edit %d %s", tui_st.current_pos + 1 /* 0-based to 1-based) */, todo_name);
  tui_st.command_multiplier = 0;
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
  { STRINGIFY(ESCAPE_KEY) , "Clear the command multiplier"                        , nv_map_clear_command_multiplier },
  { "d"                   , "Delete the selected ToDos"                           , nv_map_delete },
  { "o"                   , "Create a new ToDo below the cursor"                  , nv_map_add_below_cursor },
  { "O"                   , "Create a new ToDo above the cursor"                  , nv_map_add_above_cursor },
  { "a"                   , "Change the name of the current ToDo"                 , nv_map_edit },
};

unsigned int nv_maps_count = sizeof(nv_maps) / sizeof(Normal_visual_map);
