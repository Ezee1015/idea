#include <ctype.h>
#include <ncurses.h>
#include <string.h>
#include <stdlib.h>

#include "tui.h"
#include "utils/list.h"
#include "todo_list.h"
#include "parser.h"

#define ESCAPE_KEY 27
#define BACKSPACE_KEY 127

#define APPLY_MULTIPLIER(command) do {                                                            \
  if (tui_st.command_multiplier == 0) {                                                           \
    command;                                                                                      \
  } else {                                                                                        \
    for (unsigned int i=0; i<tui_st.command_multiplier && i < list_size(todo_list); i++) command; \
    tui_st.command_multiplier = 0;                                                                \
  }                                                                                               \
} while (0)

Point area_start = {0};
Size area_size = {0};
Tui_state tui_st = {0};

void draw_window(void) {
  int status_line_height = 2;
  const char *cursor = "--> ";
  const char *command = ":";
  const char *visual = "-- VISUAL --";

  if (tui_st.command_multiplier) {
    mvprintw(area_start.y, area_start.x, "%u", tui_st.command_multiplier);
  }

  if (tui_st.mode == MODE_COMMAND) {
    mvprintw(area_start.y, area_start.x, "%s", command);
  }

  if (tui_st.mode == MODE_VISUAL) {
    mvprintw(area_start.y, area_start.x + (area_size.width - strlen(visual))/2, "%s", visual);
  }

  // Print all items
  List_iterator iterator = list_iterator_create(todo_list);
  while (list_iterator_next(&iterator)) {
    const Todo *todo = list_iterator_element(iterator);
    const unsigned int index = list_iterator_index(iterator);
    const bool is_selected = list_contains(tui_st.selected, todo);

    if (is_selected) attron(A_REVERSE);

    mvprintw(area_start.y+index + status_line_height,
             area_start.x,
             "%d) %s", index + 1, todo->name);


    if (is_selected) attroff(A_REVERSE);
  }

  // Cursor
  mvprintw(area_start.y+ tui_st.current_pos + status_line_height,
           area_start.x - strlen(cursor),
           "%s", cursor);

  if (tui_st.mode == MODE_COMMAND) {
    move(area_start.y, area_start.x + strlen(command));
  }
}

void message(char *title, char *msg) {
  unsigned int text_width = strlen(msg) + 6;
  Point start_text = {
    .x = area_start.x + (area_size.width - text_width)/2,
    .y = area_start.y + (area_size.height - 1)/2
  };

  erase();
  mvprintw(start_text.y, start_text.x, "%s: %s", title, msg);
  getch();
  erase();
  draw_window();
}

bool parse_command(WINDOW *win, bool *exit_loop) {
  unsigned int i = 0;
  bool read = true;
  char input[256] = {0};
  char c = 0;

  while (read) {
    c = getch();

    unsigned int chars_to_clear = 0;
    if (c == BACKSPACE_KEY) {
      chars_to_clear = 2; // for the ^? symbol of backspace
      if (i) {
        input[i--] = '\0';
        chars_to_clear++; // character to remove
      }
    } else if (isalnum(c) || c == ' ') {
      input[i++] = c;
    } else { // Not recognized character, so clear the character that was printed on the screen
      chars_to_clear = 1;
    }

    if (chars_to_clear) {
      int x = getcurx(win)-chars_to_clear, y = getcury(win);
      for (unsigned int i=0; i < chars_to_clear; i++) mvprintw(y, x+i, " ");
      move(y, x);
    }

    read = (i < sizeof(input)-1)
           && (c != ESCAPE_KEY)
           && (c != '\n');
  }
  input[i] = '\0';

  if (c == ESCAPE_KEY || !strcmp(input, "")) {
    tui_st.mode = MODE_NORMAL;
    return true;
  }

  if (!strcmp(input, "q")) {
    *exit_loop = true;
    return true;
  }

  if (input[0] != '\0') {
    tui_st.mode = MODE_NORMAL;

    Input cmd = {
      .input = input,
      .length = strlen(input),
      .cursor = 0,
    };
    char *instruction = next_token(&cmd, ' ');

    Action_return (*function)(Input *input) = search_functionality_function(instruction, todo_list_functionality, todo_list_functionality_count);
    free(instruction);

    Action_return action_return = (function) ? function(&cmd) : ACTION_RETURN(RETURN_ERROR, "Invalid command");
    switch (action_return.type) {
      case RETURN_INFO:
      case RETURN_SUCCESS:
        todo_list_modified = true;
        if (action_return.message && strcmp(action_return.message, ""))
          message("INFO", action_return.message);
        break;

      case RETURN_ERROR:
        if (action_return.message && strcmp(action_return.message, ""))
          message("ERROR", action_return.message);
        break;

      case RETURN_ERROR_AND_EXIT:
        todo_list_modified = false;
        if (action_return.message && strcmp(action_return.message, ""))
          message("ERROR. Aborting...", action_return.message);
        endwin();
        return false;
    }
  }

  return true;
}

bool is_current_item_selected() {
  Todo *todo = list_get(todo_list, tui_st.current_pos);
  return list_contains(tui_st.selected, todo);
}

bool select_current_item() {
  Todo *todo = list_get(todo_list, tui_st.current_pos);
  if (list_contains(tui_st.selected, todo)) return false;
  list_append(&tui_st.selected, todo);
  return true;
}

bool unselect_current_item() {
  Todo *todo = list_get(todo_list, tui_st.current_pos);
  return (list_remove_element(&tui_st.selected, todo));
}

void toggle_select_item() {
  if (!unselect_current_item()) select_current_item();
}

void next_position() {
  if (tui_st.current_pos < list_size(todo_list)-1) tui_st.current_pos++;
}

void previous_position() {
  if (tui_st.current_pos > 0) tui_st.current_pos--;
}

bool move_selected(int direction) { // direction should be 1 or -1
  if (direction != 1 && direction != -1) abort();
  if (list_is_empty(tui_st.selected)) return false;

  Todo *limit_element = (direction == 1)
                        ? list_get(todo_list, list_size(todo_list)-1)
                        : list_get(todo_list, 0);

  if (list_contains(tui_st.selected, limit_element)) return false;

  int block_start = -1;
  for (unsigned int i=0; i<list_size(todo_list); i++) {
    Todo *e = list_get(todo_list, i);
    if (!list_contains(tui_st.selected, e)) {
      if (block_start == -1) continue;

      list_move_chunk(&todo_list, block_start, i-block_start, direction);
      block_start = -1;
      continue;
    }

    if (block_start == -1) {
      block_start = i;
    }
  }

  if (block_start != -1) {
    list_move_chunk(&todo_list, block_start, (list_size(todo_list) - 1)-block_start + 1, direction);
  }

  return true;
}

void visual_move_cursor(int direction) { // direction should be 1 or -1
  if (direction != 1 && direction != -1) abort();
  if (tui_st.mode != MODE_VISUAL) abort();

  const bool is_opposing_the_flow = (direction == 1)
                                    ? tui_st.current_pos < tui_st.visual_start_pos
                                    : tui_st.current_pos > tui_st.visual_start_pos;
  if (is_opposing_the_flow) {
    switch (tui_st.visual_mode) {
      case VISUAL_SELECT:   unselect_current_item(); break;
      case VISUAL_UNSELECT: select_current_item();   break;
    }

    if (direction == 1) next_position();
    else previous_position();
  } else {
    if (direction == 1) next_position();
    else previous_position();

    switch (tui_st.visual_mode) {
      case VISUAL_SELECT:   select_current_item();   break;
      case VISUAL_UNSELECT: unselect_current_item(); break;
    }
  }
}

void parse_normal(bool *exit_loop) {
  char input = getch();

  switch (input) {
    case ' ':
      APPLY_MULTIPLIER({
        toggle_select_item();
        next_position();
      });
      break;

    case 'j':
      APPLY_MULTIPLIER({
        switch (tui_st.mode) {
          case MODE_NORMAL:  next_position();       break;
          case MODE_VISUAL:  visual_move_cursor(1); break;
          case MODE_COMMAND: break; // unreachable
        }
      });
      break;

    case 'k':
      APPLY_MULTIPLIER({
        switch (tui_st.mode) {
          case MODE_NORMAL: previous_position();    break;
          case MODE_VISUAL: visual_move_cursor(-1); break;
          case MODE_COMMAND: break; // unreachable
        }
      });
      break;

    case 'g':
      switch (tui_st.mode) {
        case MODE_NORMAL: while (move_selected(-1));                         break;
        case MODE_VISUAL: while (tui_st.current_pos) visual_move_cursor(-1); break;
        case MODE_COMMAND: break; // unreachable
      }
      tui_st.command_multiplier = 0;
      break;

    case 'G':
      if (list_is_empty(todo_list)) break;

      switch (tui_st.mode) {
        case MODE_NORMAL: while (move_selected(1));                                                  break;
        case MODE_VISUAL: while (tui_st.current_pos < list_size(todo_list)-1) visual_move_cursor(1); break;
        case MODE_COMMAND: break; // unreachable
      }
      tui_st.command_multiplier = 0;
      break;

    case 'q':
      *exit_loop = true;
      break;

    case 'V':
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

        case MODE_COMMAND: break; // unreachable
      }
      break;

    case 'J':
      APPLY_MULTIPLIER({
          if (move_selected(1)) {
            next_position();
            if (tui_st.mode == MODE_VISUAL) tui_st.visual_start_pos++;
            todo_list_modified = true;
          }
      });
      break;

    case 'K':
      APPLY_MULTIPLIER({
          if (move_selected(-1)) {
            previous_position();
            if (tui_st.mode == MODE_VISUAL) tui_st.visual_start_pos--;
            todo_list_modified = true;
          }
      });
      break;

    case 'u':
      if (tui_st.mode == MODE_NORMAL) list_destroy(&tui_st.selected, NULL);
      tui_st.command_multiplier = 0;
      break;

    case ':':
      if (tui_st.mode == MODE_NORMAL) tui_st.mode = MODE_COMMAND;
      tui_st.command_multiplier = 0;
      break;

    case ESCAPE_KEY:
      if (tui_st.command_multiplier) tui_st.command_multiplier = 0;
      break;

    default:
      if (isdigit(input)) tui_st.command_multiplier = (tui_st.command_multiplier * 10) + (input-'0');
      break;

  }
}

bool window_app(void) {
  WINDOW *win = initscr();
  curs_set(0);

  Size window_size = {0};
  Size old_dimension = {0};
  bool exit_loop = false;

  do {
    window_size.width = getmaxx(win);
    window_size.height = getmaxy(win);

    if (window_size.width < area_size.width) {
      erase();
      printw("Window is not width enough...");
    } else if (window_size.height < area_size.height) {
      erase();
      printw("Window is not height enough...");
    } else {
      if (old_dimension.width != window_size.width || old_dimension.height != window_size.height) {
        old_dimension.width = window_size.width;
        old_dimension.height = window_size.height;
        area_size.width = window_size.width/2;
        area_size.height = window_size.height/2;
        area_start.x = (window_size.width-area_size.width)/2;
        area_start.y = (window_size.height-area_size.height)/2;
      }

      erase();
      draw_window();

      if (tui_st.mode == MODE_COMMAND) {
        if (!parse_command(win, &exit_loop)) return false;
      } else {
        parse_normal(&exit_loop);
      }
    }
  } while ( !exit_loop );

  return (endwin() != ERR);
}
