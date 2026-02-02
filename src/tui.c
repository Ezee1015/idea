#include <ctype.h>
#include <ncurses.h>
#include <string.h>
#include <stdlib.h>

#include "tui.h"
#include "utils/list.h"
#include "utils/string.h"
#include "parser.h"
#include "tui_mappings.h"

#define APPLY_MULTIPLIER(command) do {                                                            \
  if (tui_st.command_multiplier == 0) {                                                           \
    command;                                                                                      \
  } else {                                                                                        \
    for (unsigned int i=0; i<tui_st.command_multiplier && i < list_size(todo_list); i++) command; \
    tui_st.command_multiplier = 0;                                                                \
  }                                                                                               \
} while (0)

#define i_div_ceil(dividend, divisor) (dividend % divisor)     \
                                      ? dividend / divisor + 1 \
                                      : dividend / divisor;

Size window_size = {0};
Point area_start = {0};
Size area_size = {0};
Tui_state tui_st = {0};
char input[256] = {0};

void draw_window(void) {
  int status_line_height = 2;
  const char *cursor = "--> ";
  const unsigned int cursor_length = strlen(cursor);
  const char *command = ":";
  const char *visual = "-- VISUAL --";
  const char *changed = "[+]";

  // Area box
  // draw_rect(area_start.y - 1,
  //           area_start.x - 1,
  //           area_start.y + area_size.height,
  //           area_start.x + area_size.width);

  if (todo_list_modified) {
    mvprintw(area_start.y, area_start.x + area_size.width - strlen(changed), "%s", changed);
  }

  if (tui_st.command_multiplier) {
    mvprintw(area_start.y, area_start.x, "%u", tui_st.command_multiplier);
  }

  if (tui_st.mode == MODE_COMMAND) {
    mvprintw(area_start.y, area_start.x + cursor_length, "%s", command);
    if (strcmp(input, "")) printw("%s", input);
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
    move(area_start.y, area_start.x + cursor_length + strlen(command) + strlen(input));
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

char message(char *title, char *msg) {
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

  return getch();
}

bool confirm(char *msg, Confirm_type type) {
  const char *msg_suffix = "Confirm?";
  String_builder confirm_msg = sb_new();

  char *options = NULL;
  switch (type) {
    case CONFIRM_NORMAL: options = "[y/n]"; break;
    case CONFIRM_DEFAULT_YES: options = "[Y/n]"; break;
    case CONFIRM_DEFAULT_NO: options = "[y/N]"; break;
  }

  sb_append_with_format(&confirm_msg, "%s\n\n%s %s", msg, msg_suffix, options);

  bool ret;
  bool valid_input = false;
  while (!valid_input) {
    char c = message("CONFIRM", confirm_msg.str);

    if (c == 'n' || c == 'N') {
      ret = false;
      valid_input = true;
    } else if (c == 'y' || c == 'Y') {
      ret = true;
      valid_input = true;
    } else if (c == ENTER_KEY) {
      switch (type) {
        case CONFIRM_NORMAL: break;
        case CONFIRM_DEFAULT_YES:
            ret = true;
            valid_input = true;
          break;

        case CONFIRM_DEFAULT_NO:
          ret = false;
          valid_input = true;
          break;
      }
    }
  }

  sb_free(&confirm_msg);
  return ret;
}

bool parse_command() {
  unsigned int i = strlen(input);
  bool read = true;
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
    } else if (isalnum(c) || c == ' ' || c == '_') {
      input[i++] = c;
    } else { // Not recognized character, so clear the character that was printed on the screen
      chars_to_clear = 1;
    }

    if (chars_to_clear) {
      int x = getcurx(stdscr)-chars_to_clear, y = getcury(stdscr);
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

  if (input[0] != '\0') {
    tui_st.mode = MODE_NORMAL;

    Input cmd = {
      .input = input,
      .length = strlen(input),
      .cursor = 0,
    };
    char *instruction = next_token(&cmd, ' ');

    Action_return (*function)(Input *input) = search_functionality_function(instruction, tui_functionality, tui_functionality_count);
    if (!function) {
      function = search_functionality_function(instruction, todo_list_functionality, todo_list_functionality_count);
      if (function) todo_list_modified = true;
    }
    free(instruction); instruction = NULL;

    Action_return action_return = (function) ? function(&cmd) : ACTION_RETURN(RETURN_ERROR, "Invalid command");
    switch (action_return.type) {
      case RETURN_INFO:
      case RETURN_SUCCESS:
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
        input[0] = '\0';
        return false;
    }

    input[0] = '\0';
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

bool delete_selected() {
  if (list_is_empty(tui_st.selected)) return false;

  String_builder msg = sb_new();
  sb_append(&msg, "ToDos to remove:");
  List_iterator iterator = list_iterator_create(tui_st.selected);
  while (list_iterator_next(&iterator)) {
    Todo *e = list_iterator_element(iterator);
    sb_append_with_format(&msg, "\n  - %s", e->name);
  }

  bool ok = confirm(msg.str, CONFIRM_DEFAULT_NO);
  sb_free(&msg);
  if (!ok) {
    message("INFO", "Delete cancelled!");
    return false;
  }

  while (!list_is_empty(tui_st.selected)) {
    Todo *e = list_remove(&tui_st.selected, 0);
    list_remove_element(&todo_list, e);
    free_todo(e);
  }

  // Reposition cursor if it's outside the bounds
  if (tui_st.current_pos > list_size(todo_list)-1) tui_st.current_pos = list_size(todo_list)-1;
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

Help_result show_functionality_message(const char *source, const unsigned source_index, const unsigned int source_count, const Functionality *functionality, const unsigned int functionality_count, bool are_commands, bool from_the_end, unsigned int *max_functionality_per_page) {
  Help_result ret = HELP_RETURN_QUIT;
  const char *prefix_cmd = (are_commands) ? ":": " ";

  unsigned int max_cmd_length = 0;
  for (unsigned int i=0; i<functionality_count; i++) {
    const Functionality f = functionality[i];
    const unsigned int abbreviated_cmd_length = (f.abbreviation_cmd) ? strlen(f.abbreviation_cmd) : 0;
    const unsigned int full_cmd_length = strlen(f.full_cmd);
    const unsigned int cmd_length = abbreviated_cmd_length + full_cmd_length;
    if (cmd_length > max_cmd_length) max_cmd_length = cmd_length;
  }

  String_builder msg = sb_new();
  unsigned int pages = i_div_ceil(functionality_count, *max_functionality_per_page);
  unsigned int page = (from_the_end) ? pages-1 : 0;
  bool exit = false;
  do {
    sb_clean(&msg);

    for (unsigned int i=page * (*max_functionality_per_page); i < (page+1) * (*max_functionality_per_page) && i<functionality_count; i++) {
      const Functionality f = functionality[i];
      const unsigned int abbreviated_cmd_length = (f.abbreviation_cmd) ? strlen(f.abbreviation_cmd) : 0;
      const unsigned int full_cmd_length = strlen(f.full_cmd);
      const unsigned int cmd_length = abbreviated_cmd_length + full_cmd_length;

      const unsigned int description_padding = 3;
      unsigned int padding = max_cmd_length - cmd_length + description_padding;

      if (i != page * (*max_functionality_per_page)) sb_append(&msg, "\n");

      if (f.abbreviation_cmd) {
        sb_append_with_format(&msg, "%s%s %s%s", prefix_cmd, f.full_cmd, prefix_cmd, f.abbreviation_cmd);
        while ( (padding--) > 0 ) sb_append(&msg, " ");
        sb_append_with_format(&msg, "%s\n", f.man.description);
      } else {
        sb_append_with_format(&msg, "%s%s  ", prefix_cmd, f.full_cmd, f.abbreviation_cmd);
        while ( (padding--) > 0 ) sb_append(&msg, " ");
        sb_append_with_format(&msg, "%s\n", f.man.description);
      }

      const unsigned int separator_between_commands_length = 2; // " :" when f.abbreviation_cmd exist or "  " when it doesn't
      for (unsigned int x = 0; f.man.parameters[x]; x++) {
        unsigned int padding_parameter = max_cmd_length + description_padding + separator_between_commands_length;
        sb_append_with_format(&msg, " ");
        while ( (padding_parameter--) > 0 ) sb_append(&msg, " ");
        sb_append_with_format(&msg, "Usage: %s %s\n", f.full_cmd, functionality[i].man.parameters[x]);
      }
    }

    if (0 <= (int)msg.length-1) msg.str[msg.length-1] = '\0'; // Remove the last '\n' from the last "Usage:"

    String_builder title = sb_new();
    sb_append_with_format(&title, "Help %u/%u: %s [page %u/%u]", source_index+1, source_count, source, page+1, pages);
    char c = message(title.str, msg.str);
    sb_free(&title);

    switch (c) {
      case 'q': exit = true; break;

      case '-':
        if (*max_functionality_per_page > 1) {
          (*max_functionality_per_page)--;
          pages = i_div_ceil(functionality_count, *max_functionality_per_page);
          if (page >= pages) page = pages - 1;
        }
        break;

      case '+':
        if (*max_functionality_per_page < functionality_count) {
          (*max_functionality_per_page)++;
          pages = i_div_ceil(functionality_count, *max_functionality_per_page);
          if (page >= pages) page = pages - 1;
        }
        break;

      case 'l':
        if (page < pages-1) {
          page++;
        } else {
          ret = HELP_RETURN_NEXT;
          exit = true;
        }
        break;

      case 'h':
        if (page > 0) {
          page--;
        } else {
          ret = HELP_RETURN_PREV;
          exit = true;
        }
        break;
    }
  } while (!exit);

  sb_free(&msg);
  return ret;
}

Action_return action_help(Input *input) {
  ACTION_NO_ARGS("help", input);

  unsigned int max_functionality_per_page = 5;

  // Create a temporary functionality list only to use the show_functionality_message()
  const Functionality tui_mappings[] = {
#define X(key, description, code_block) { (char [2]){key, '\0'}, NULL, NULL, MAN(description, NULL)},
    MAPPINGS()
#undef X

   { "[1-9]", NULL, NULL, MAN("Number of times to repeat the command (command multiplier)", NULL) },
  };
  const unsigned int tui_mappings_count = sizeof(tui_mappings) / sizeof(tui_mappings[0]);

  const struct {
    const char *name;
    const Functionality *func;
    unsigned int func_count;
    bool are_commands;
  } functs[] = {
    { "Generic commands", todo_list_functionality, todo_list_functionality_count, true },
    { "TUI commands", tui_functionality, tui_functionality_count, true },
    { "TUI mappings", tui_mappings, tui_mappings_count, false }
  };
  const unsigned int functs_count = sizeof(functs) / sizeof(functs[0]);

  Help_result r;
  unsigned int i = 0;
  bool from_the_end = false;
  do {
    r = show_functionality_message(functs[i].name, i, functs_count, functs[i].func, functs[i].func_count, functs[i].are_commands, from_the_end, &max_functionality_per_page);

    switch (r) {
      case HELP_RETURN_NEXT:
        if (i < functs_count-1) {
          i++;
          from_the_end = false;
        } else {
          from_the_end = true;
        }
        break;

      case HELP_RETURN_PREV:
        if (0 < i) {
          i--;
          from_the_end = true;
        } else {
          from_the_end = false;
        }
        break;

      case HELP_RETURN_QUIT: break;
    }
  } while (r != HELP_RETURN_QUIT);

  return ACTION_RETURN(RETURN_SUCCESS, "");
}

#ifdef COMMIT
#include "main.h"
Action_return action_tui_version(Input *input) {
  ACTION_NO_ARGS("version", input);

  String_builder sb = sb_new();
  sb_append(&sb, "Version: " COMMIT "\n");
  sb_append_with_format(&sb, "Config: %s\n", idea_state.local_path);
  message("Version", sb.str);
  sb_free(&sb);
  return ACTION_RETURN(RETURN_SUCCESS, "");
}
#endif // COMMIT

Action_return action_quit(Input *input) {
  ACTION_NO_ARGS("quit", input);

  bool ok = confirm("Quit without saving?", CONFIRM_DEFAULT_NO);
  if (ok) {
    tui_st.exit_loop = true;
    todo_list_modified = false;
    return ACTION_RETURN(RETURN_SUCCESS, "");
  }

  return ACTION_RETURN(RETURN_INFO, "Operation canceled");
}

Action_return action_save_and_quit(Input *input) {
  ACTION_NO_ARGS("save & quit", input);

  // The list is going to be save after the loop if it was modified
  tui_st.exit_loop = true;
  return ACTION_RETURN(RETURN_SUCCESS, "");
}

Action_return action_save(Input *input) {
  ACTION_NO_ARGS("save", input);

  if (!save_todo_list()) return ACTION_RETURN(RETURN_ERROR, "Unable to save the ToDo list");

  todo_list_modified = false;
  return ACTION_RETURN(RETURN_SUCCESS, "");
}

Action_return action_add_at_todo_tui(Input *input) {
  Action_return (*function)(Input *input) = search_functionality_function("add_at", todo_list_functionality, todo_list_functionality_count);
  if (!function) abort(); // Should not happen

  // Peek the index from the input
  const unsigned int argument_cursor = input->cursor;
  char *pos_str = next_token(input, ' ');
  input->cursor = argument_cursor;

  Action_return action_return = function(input);
  unsigned int pos = 0;
  switch (action_return.type) {
    case RETURN_INFO:
    case RETURN_SUCCESS:
      todo_list_modified = true;
      pos = atoi(pos_str);
      free(pos_str);
      if (!pos) abort(); // Should not happen as this is checked inside the original add_at (todo_list.c)

      tui_st.current_pos = pos-1;
      return ACTION_RETURN(RETURN_SUCCESS, action_return.message);

    case RETURN_ERROR:
    case RETURN_ERROR_AND_EXIT:
      free(pos_str);
      return action_return;
  }

  return ACTION_RETURN(RETURN_ERROR, "Unreachable");
}

Functionality tui_functionality[] = {
  { "add_at", "ap", action_add_at_todo_tui, MAN("Add a ToDo in the specified position", "[index] [name]") },
  { "write", "w", action_save, MAN("Write the changes to disk", NULL) },
  { "quit", "q", action_quit, MAN("Exit the application without saving the changes", NULL) },
  { "write_quit", "wq", action_save_and_quit, MAN("Save the changes and exit", NULL) },
  { "help", "h", action_help, MAN("See the help menu", NULL) },
#ifdef COMMIT
  { "version", "ver", action_tui_version, MAN("Print the commit hash and version", NULL) },
#endif // COMMIT
};
unsigned int tui_functionality_count = sizeof(tui_functionality) / sizeof(Functionality);

void parse_normal() {
  char c = getch();

  switch (c) {
#define X(key, description, code_block) case key: code_block; break;
    MAPPINGS()
#undef X

    default:
      if (isdigit(c)) tui_st.command_multiplier = (tui_st.command_multiplier * 10) + (c-'0');
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

#define GET_MINIMUM_HEIGHT() list_size(todo_list) + 2
bool window_app(void) {
  WINDOW *win = initscr();
  curs_set(0);

  Size minimum_window_size = { .width = 35, .height = GET_MINIMUM_HEIGHT() };

  Size old_dimension = {0};
  unsigned int old_todo_list_size = -1; // '-1' makes it update the first start
  do {
    window_size.width = getmaxx(win);
    window_size.height = getmaxy(win);

    if (window_size.width < minimum_window_size.width) {
      erase();
      printw("Window is not width enough...");
      char c = getch(); // This captures when the screen resize too
      if (c == 'q') tui_st.exit_loop = true;
    } else if (window_size.height < minimum_window_size.height) {
      erase();
      printw("Window is not height enough...");
      char c = getch(); // This captures when the screen resize too
      if (c == 'q') tui_st.exit_loop = true;

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
        minimum_window_size.height = GET_MINIMUM_HEIGHT();
        update_area_y_axis();
      }

      erase();

      draw_window();

      if (tui_st.mode == MODE_COMMAND) {
        if (!parse_command()) return false;
      } else {
        parse_normal();
      }
    }
  } while ( !tui_st.exit_loop );

  return (endwin() != ERR);
}
