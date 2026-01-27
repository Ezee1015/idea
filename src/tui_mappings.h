#ifndef MAPPINGS

#define ESCAPE_KEY 27
#define BACKSPACE_KEY 127
#define ENTER_KEY 10

// X(key, description, code_block)
#define MAPPINGS() \
\
  X(' ', "Toggle select", { \
    APPLY_MULTIPLIER({      \
      toggle_select_item(); \
      next_position();      \
    });                     \
  })                        \
\
  X('j', "Move the cursor down", {                       \
    APPLY_MULTIPLIER({                                   \
      switch (tui_st.mode) {                             \
        case MODE_NORMAL:  next_position();       break; \
        case MODE_VISUAL:  visual_move_cursor(1); break; \
        case MODE_COMMAND: break; /* unreachable */      \
      }                                                  \
    });                                                  \
  })                                                     \
\
  X('k', "Move the cursor up", {                         \
    APPLY_MULTIPLIER({                                   \
      switch (tui_st.mode) {                             \
        case MODE_NORMAL: previous_position();    break; \
        case MODE_VISUAL: visual_move_cursor(-1); break; \
        case MODE_COMMAND: break; /* unreachable */      \
      }                                                  \
    });                                                  \
  })                                                     \
\
  X('g', "Move the cursor and the selected items to the top", {                   \
    switch (tui_st.mode) {                                                        \
      case MODE_NORMAL:                                                           \
        if (!list_is_empty(tui_st.selected)) {                                    \
          while (move_selected(-1));                                              \
        }                                                                         \
        while (previous_position());                                              \
        break;                                                                    \
      case MODE_VISUAL: while (tui_st.current_pos) visual_move_cursor(-1); break; \
      case MODE_COMMAND: break; /* unreachable */                                 \
    }                                                                             \
    tui_st.command_multiplier = 0;                                                \
  })                                                                              \
\
  X('G', "Move the cursor and the selected items to the bottom", {                                        \
    if (!list_is_empty(todo_list)) break;                                                                 \
                                                                                                          \
    switch (tui_st.mode) {                                                                                \
      case MODE_NORMAL:                                                                                   \
        if (!list_is_empty(tui_st.selected)) {                                                            \
          while (move_selected(1));                                                                       \
        }                                                                                                 \
        while (next_position());                                                                          \
        break;                                                                                            \
      case MODE_VISUAL: while (tui_st.current_pos < list_size(todo_list)-1) visual_move_cursor(1); break; \
      case MODE_COMMAND: break; /* unreachable  */                                                        \
    }                                                                                                     \
    tui_st.command_multiplier = 0;                                                                        \
  })                                                                                                      \
\
  X('s', "Save the changes to disk (same as :w)", {                \
    Action_return ret = action_save(NULL);                         \
    if (ret.type != RETURN_SUCCESS) message("ERROR", ret.message); \
  })                                                               \
  \
  X('W', "Save and Exit idea (same as :wq)", {                     \
    Action_return ret = action_save_and_quit(NULL);                \
    if (ret.type != RETURN_SUCCESS) message("ERROR", ret.message); \
    })                                                             \
\
  X('Q', "Exit idea without saving (same as :q)", {                \
    Action_return ret = action_quit(NULL);                         \
    if (ret.type != RETURN_SUCCESS) message("ERROR", ret.message); \
  })                                   \
\
  X('q', "Confirm to save and Exit idea (similar to :q but asks whether to save)", { \
    if (todo_list_modified) {                                                  \
      bool save = confirm("Save?", CONFIRM_DEFAULT_YES);                       \
      if (!save) todo_list_modified = false;                                   \
    }                                                                          \
                                                                               \
    tui_st.exit_loop = true;                                                   \
  })                                                                           \
\
  X('V', "Enter or Exit visual mode", {                                        \
    switch (tui_st.mode) {                                                     \
      case MODE_NORMAL:                                                        \
        tui_st.visual_start_pos = tui_st.current_pos;                          \
        tui_st.mode = MODE_VISUAL;                                             \
        tui_st.visual_mode = (is_current_item_selected())                      \
                             ? VISUAL_UNSELECT                                 \
                             : VISUAL_SELECT;                                  \
                                                                               \
        APPLY_MULTIPLIER({                                                     \
          switch (tui_st.visual_mode) {                                        \
            case VISUAL_SELECT:   select_current_item();   break;              \
            case VISUAL_UNSELECT: unselect_current_item(); break;              \
          }                                                                    \
          next_position();                                                     \
        });                                                                    \
                                                                               \
        if (tui_st.current_pos != list_size(todo_list)-1) previous_position(); \
        break;                                                                 \
                                                                               \
      case MODE_VISUAL:                                                        \
        tui_st.mode = MODE_NORMAL;                                             \
        tui_st.command_multiplier = 0;                                         \
        break;                                                                 \
                                                                               \
      case MODE_COMMAND: break; /* unreachable */                              \
    }                                                                          \
  })                                                                           \
\
  X('J', "Move the selected ToDos down", {                           \
    APPLY_MULTIPLIER({                                               \
        if (move_selected(1)) {                                      \
          next_position();                                           \
          if (tui_st.mode == MODE_VISUAL) tui_st.visual_start_pos++; \
          todo_list_modified = true;                                 \
        }                                                            \
    });                                                              \
    break;                                                           \
  })                                                                 \
\
  X('K', "Move the selected ToDos up", {                             \
    APPLY_MULTIPLIER({                                               \
        if (move_selected(-1)) {                                     \
          previous_position();                                       \
          if (tui_st.mode == MODE_VISUAL) tui_st.visual_start_pos--; \
          todo_list_modified = true;                                 \
        }                                                            \
    });                                                              \
  })                                                                 \
\
  X('u', "Unselect all the selected items", {                             \
    if (tui_st.mode == MODE_NORMAL) list_destroy(&tui_st.selected, NULL); \
    tui_st.command_multiplier = 0;                                        \
    break;                                                                \
  })                                                                      \
\
  X(':', "Enter command mode", {                                \
    if (tui_st.mode == MODE_NORMAL) tui_st.mode = MODE_COMMAND; \
    tui_st.command_multiplier = 0;                              \
  })                                                            \
\
  X(ESCAPE_KEY, "Clear the command multiplier", {                 \
    if (tui_st.command_multiplier) tui_st.command_multiplier = 0; \
  })                                                              \
\
  X('d', "Delete the selected ToDos", {                                \
    if (tui_st.mode == MODE_NORMAL) {                                  \
      /* if (list_is_empty(tui_st.selected)) select_current_item(); */ \
                                                                       \
      if (delete_selected()) todo_list_modified = true;                \
    }                                                                  \
  })                                                                   \
\
  X('o', "Create a new ToDo below the cursor", {                                                                       \
    tui_st.mode = MODE_COMMAND;                                                                                        \
    snprintf(input, sizeof(input), "add_at %d ", tui_st.current_pos + 1 /* 0-based to 1-based) */ + 1 /* next pos */); \
    tui_st.current_pos++;                                                                                              \
    tui_st.command_multiplier = 0;                                                                                     \
  })                                                                                                                   \
\
  X('O', "Create a new ToDo above the cursor", {                                                    \
    tui_st.mode = MODE_COMMAND;                                                                     \
    snprintf(input, sizeof(input), "add_at %d ", tui_st.current_pos + 1 /* 0-based to 1-based) */); \
    tui_st.command_multiplier = 0;                                                                  \
  })

#endif // MAPPINGS
