#include <string.h>
#include <stdlib.h>

#include "tui.h"
#include "utils/list.h"
#include "db.h"
#include "actions.h"

Point area_start = {0};
Size area_size = {0};

void draw_window(void) {
  List_iterator iterator = list_iterator_create(todo_list);
  while (list_iterator_next(&iterator)) {
    Todo *todo = list_iterator_element(iterator);
    mvprintw(area_start.y+list_iterator_index(iterator) + 2,
             area_start.x,
             "%d) %s", list_iterator_index(iterator)+1, todo->data); // +2 for the command line
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
        Action_return action_return = action(input);
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
