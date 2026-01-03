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

Size window_size = {0};
Point area_start = {0};
Size area_size = {0};
Tui_state tui_st = {0};

void draw_window(void) {
  int status_line_height = 2;
  const char *cursor = "--> ";
  const unsigned int cursor_length = strlen(cursor);
  const char *command = ":";
  const char *visual = "-- VISUAL --";

  if (tui_st.command_multiplier) {
    mvprintw(area_start.y, area_start.x, "%u", tui_st.command_multiplier);
  }

  if (tui_st.mode == MODE_COMMAND) {
    mvprintw(area_start.y, area_start.x + cursor_length, "%s", command);
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
             area_start.x + cursor_length,
             "%d) %s", index + 1, todo->name);


    if (is_selected) attroff(A_REVERSE);
  }

  // Cursor
  if (list_size(todo_list)) {
    mvprintw(area_start.y + tui_st.current_pos + status_line_height,
        area_start.x,
        "%s", cursor);
  }

  if (tui_st.mode == MODE_COMMAND) {
    move(area_start.y, area_start.x + cursor_length + strlen(command));
  }
}

void draw_rect(int y1, int x1, int y2, int x2) {
    mvhline(y1, x1, 0, x2-x1);
    mvhline(y2, x1, 0, x2-x1);
    mvvline(y1, x1, 0, y2-y1);
    mvvline(y1, x2, 0, y2-y1);
    mvaddch(y1, x1, ACS_ULCORNER);
    mvaddch(y2, x1, ACS_LLCORNER);
    mvaddch(y1, x2, ACS_URCORNER);
    mvaddch(y2, x2, ACS_LRCORNER);
}

void message(char *title, char *msg) {
  const Size padding = { .width = 3, .height = 1 };

  const unsigned int line_length_limit = area_size.width - 2 /* border */ - padding.width * 2;
  const unsigned int msg_len = strlen(msg);

  unsigned int lines = 1;
  unsigned int max_line_length = 0;

  unsigned int line_length = 0;
  for (unsigned int i=0; i<msg_len; i++) {
    if (msg[i] == '\n' || line_length == line_length_limit) {
      lines++;
      line_length = 0;
    }
    if (msg[i] != '\n') line_length++;
    if (line_length > max_line_length) max_line_length = line_length;
  }

  Size box_size = {
    .width = max_line_length + padding.width * 2,
    .height = 4 /* 2 (box borders) + 1 (title line) + 1 (title) */ + padding.height * 2 + lines,
  };

  // I need to cast the subtraction to int because if the box is bigger than the
  // area_size it would overflow the unsigned int operation between area_size
  // and box_size and break the box.
  Point box_start = {
    .x = area_start.x + (int)(area_size.width - box_size.width)/2,
    .y = area_start.y + (int)(area_size.height - box_size.height)/2
  };
  // Fix box_start (unsigned int) overflow
  if (box_start.x > window_size.width) box_start.x = 0;
  if (box_start.y > window_size.height) box_start.y = 0;

  erase();

  // Draw the box
  draw_rect(
      box_start.y,
      box_start.x,
      box_start.y + box_size.height - 1,
      box_start.x + box_size.width - 1
  );
  // Title bar
  mvhline(box_start.y + 2,
          box_start.x,
          0,
          box_size.width - 1);
  mvaddch(box_start.y + 2,
          box_start.x,
          ACS_LTEE);
  mvaddch(box_start.y + 2,
          box_start.x + box_size.width - 1,
          ACS_RTEE);

  mvprintw(box_start.y + 1,
           box_start.x + (box_size.width - strlen(title))/2,
           "%s", title);

  const unsigned int start_y = box_start.y + 1 + padding.height + 2,
                     start_x = box_start.x + padding.width;

  unsigned int cur_y = start_y,
               cur_x = start_x;
  move(cur_y, cur_x);
  for (unsigned int i=0; i<msg_len; i++) {
    if (msg[i] == '\n' || (cur_x - start_x) == line_length_limit) {
      cur_y++;
      cur_x = start_x;

      move(cur_y, cur_x);
    }

    if (msg[i] != '\n') {
      printw("%c", msg[i]);
      cur_x++;
    }
  }

  getch();
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

bool next_position() {
  if (list_size(todo_list) == 0 || tui_st.current_pos >= list_size(todo_list)-1) return false;
  tui_st.current_pos++;
  return true;
}

bool previous_position() {
  if (tui_st.current_pos == 0) return false;
  tui_st.current_pos--;
  return true;
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
        case MODE_NORMAL:
          if (!list_is_empty(tui_st.selected)) {
            while (move_selected(-1));
          }
          while (previous_position());
          break;
        case MODE_VISUAL: while (tui_st.current_pos) visual_move_cursor(-1); break;
        case MODE_COMMAND: break; // unreachable
      }
      tui_st.command_multiplier = 0;
      break;

    case 'G':
      if (list_is_empty(todo_list)) break;

      switch (tui_st.mode) {
        case MODE_NORMAL:
          if (!list_is_empty(tui_st.selected)) {
            while (move_selected(1));
          }
          while (next_position());
          break;
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

void update_area_x_axis() {
  area_size.width = window_size.width/2;
  area_start.x = (window_size.width-area_size.width)/2;
}

void update_area_y_axis() {
  area_size.height = list_size(todo_list) + 2; // + 2 for the status line
  area_start.y = (window_size.height-area_size.height)/2;
}

bool window_app(void) {
  WINDOW *win = initscr();
  curs_set(0);

  Size minimum_window_size = { .width = 35, .height = list_size(todo_list) + 2 };
  bool exit_loop = false;

  Size old_dimension = {0};
  unsigned int old_todo_list_size = -1; // '-1' makes it update the first start
  do {
    window_size.width = getmaxx(win);
    window_size.height = getmaxy(win);

    if (window_size.width < minimum_window_size.width) {
      erase();
      printw("Window is not width enough...");
      char c = getch(); // This captures when the screen resize too
      if (c == 'q') exit_loop = true;
    } else if (window_size.height < minimum_window_size.height) {
      erase();
      printw("Window is not height enough...");
      char c = getch(); // This captures when the screen resize too
      if (c == 'q') exit_loop = true;

    } else {
      if (old_dimension.width != window_size.width) {
        old_dimension.width = window_size.width;
        update_area_x_axis();
      }

      if (old_dimension.height != window_size.height) {
        old_dimension.height = window_size.height;
        update_area_y_axis();
      }

      if (old_todo_list_size != list_size(todo_list)) {
        old_todo_list_size = list_size(todo_list);
        minimum_window_size.height = list_size(todo_list) + 2;
        update_area_y_axis();
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
