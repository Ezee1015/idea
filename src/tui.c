#include <string.h>
#include <stdlib.h>

#include "tui.h"
#include "utils/list.h"
#include "todo_list.h"
#include "parser.h"

Point area_start = {0};
Size area_size = {0};

void draw_window(void) {
  List_iterator iterator = list_iterator_create(todo_list);
  while (list_iterator_next(&iterator)) {
    const Todo *todo = list_iterator_element(iterator);
    const unsigned int index = list_iterator_index(iterator);

    mvprintw(area_start.y+index + 2, // +2 for the command line
             area_start.x,
             "%d) %s", index + 1, todo->name);
  }

  mvprintw(area_start.y, area_start.x, "> ");
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

int window_app(void) {
  WINDOW *win = initscr();
  curs_set(0);

  Size window_size = {0};
  Size old_dimension = {0};
  char input[256] = {0};

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

      if (input[0] != '\0') {
        Input cmd = {
          .input = input,
          .length = strlen(input),
          .cursor = 0,
        };
        char *instruction = next_token(&cmd, ' ');

        // TODO Implement tui custom functionality: help (Generic and TUI specific commands)
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
            return 1;
        }
      }

      erase();
      draw_window();
    }

    input[0] = '\0';
    int ret = getnstr(input, sizeof(input)-1);
    if (ret != OK && ret != KEY_RESIZE) abort();
  } while ( strcmp(input, "q") );

  return (endwin() == ERR);
}
