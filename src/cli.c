#include <stdlib.h>
#include <string.h>

#include "cli.h"
#include "parser.h"
#include "todo_list.h"

/// Functionality
Action_return print_todo(Input *input) {
  List_iterator iterator = list_iterator_create(todo_list);
  while (list_iterator_next(&iterator)) {
    Todo *todo = list_iterator_element(iterator);
    printf("%d) %s\n", list_iterator_index(iterator)+1, todo->data);
  }
  return ACTION_RETURN(RETURN_SUCCESS, "");
}

Functionality cli_functionality[] = {
  {"-l", "list", print_todo},
};

unsigned int cli_functionality_count() {
  return sizeof(cli_functionality) / sizeof(Functionality);
}

/// Parsing

void cli_parse_input(int argc, char *argv[]) {
  for (int i = 1; i<argc; i++) {
    Input cmd = {
      .input = argv[i],
      .length = strlen(argv[i]),
      .cursor = 0,
    };
    char *instruction = next_token(&cmd, ' ');

    Action_return (*function)(Input *input) = search_functionality_pos(instruction, cli_functionality, cli_functionality_count());
    if (!function) {
      function = search_functionality_pos(instruction, todo_list_functionality, todo_list_functionality_count());
      todo_list_modified = true;
    }
    free(instruction);

    Action_return action_return = (function) ? function(&cmd) : ACTION_RETURN(RETURN_ERROR, "Invalid command");
    switch (action_return.type) {
      case RETURN_INFO:
      case RETURN_SUCCESS:
        if (cmd.cursor <= cmd.length) abort(); // Command parsing error. There's data left in the command

        if (action_return.message && strcmp(action_return.message, ""))
          printf("INFO: %s\n", action_return.message);
        break;

      case RETURN_ERROR:
        todo_list_modified = false; // in order to not save the list
        if (action_return.message && strcmp(action_return.message, ""))
          printf("ERROR: %s\n", action_return.message);
        break;

      case RETURN_ERROR_AND_EXIT:
        todo_list_modified = false; // in order to not save the list
        if (action_return.message && strcmp(action_return.message, ""))
          printf("ABORT: %s\n", action_return.message);
        return;
    }
  }

  if (todo_list_modified) print_todo(NULL);
}
