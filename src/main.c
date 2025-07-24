#include <string.h>

#include "todo_list.h"
#include "tui.h"
#include "cli.h"

int main(int argc, char *argv[]) {
  load_file();

  if (argc == 1) {
    // TUI Version
    window_app();
  } else {
    // CLI Version
    for (int i=1; i<argc; i++) {
      Action_return result = cli_parse_input(argv[i]);
      switch (result.type) {
        case RETURN_SUCCESS:
        case RETURN_INFO:
          if (result.message && strcmp(result.message, ""))
            printf("Message from the %dº command (%s)\n\n", i, argv[i]);
          break;

        case RETURN_ERROR:
          list_destroy(&todo_list, (void (*)(void *))free_todo);
          printf("An ERROR occurred in the %dº command (%s)\n", i, argv[i]);
          return 1;

      case RETURN_ERROR_AND_EXIT:
          list_destroy(&todo_list, (void (*)(void *))free_todo);
          printf("An ABORT occurred in the %dº command (%s)\n", i, argv[i]);
          return 1;
      }
    }
    if (todo_list_modified) print_todo(NULL);
  }

  if (todo_list_modified) save_file();
  list_destroy(&todo_list, (void (*)(void *))free_todo);
  return 0;
}
