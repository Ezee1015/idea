#include <stdlib.h>
#include <string.h>

#include "cli.h"
#include "parser.h"
#include "todo_list.h"
#include "utils/list.h"

/// Functionality
Action_return print_todo(Input *input) {
  List_iterator iterator = list_iterator_create(todo_list);
  while (list_iterator_next(&iterator)) {
    Todo *todo = list_iterator_element(iterator);
    printf("%d) %s\n", list_iterator_index(iterator)+1, todo->data);
  }
  return ACTION_RETURN(RETURN_SUCCESS, "");
}

Action_return export_todo(Input *input) {
  if (list_is_empty(todo_list)) return ACTION_RETURN(RETURN_ERROR, "Unable to create the export file: the todo list is empty.");

  char *export_path = next_token(input, '\0');
  if (!export_path) return ACTION_RETURN(RETURN_ERROR, "Command malformed");

  FILE *export_file = fopen(export_path, "w");
  free(export_path);
  if (!export_file) return ACTION_RETURN(RETURN_ERROR, "Unable to create the export file");

  if (!export_template(export_file)) {
    fclose(export_file);
    return ACTION_RETURN(RETURN_ERROR, "Unable to save todo's structure info");
  }
  fprintf(export_file, "clear all\n");
  List_iterator iterator = list_iterator_create(todo_list);
  while (list_iterator_next(&iterator)) {
    if (!save_todo_to_text_file(export_file, list_iterator_element(iterator))) {
      fclose(export_file);
      return ACTION_RETURN(RETURN_ERROR, "Unable to save todo");
    }
  }

  fclose(export_file);
  return ACTION_RETURN(RETURN_SUCCESS, "");
}

Action_return import_todo(Input *input) {
  char *import_path = next_token(input, '\0');
  if (!import_path) return ACTION_RETURN(RETURN_ERROR, "Command malformed");

  FILE *import_file = fopen(import_path, "r");
  free(import_path);
  if (!import_file) return ACTION_RETURN(RETURN_ERROR, "Unable to open the import file");

  char buffer[1024];
  unsigned int line = 1;
  while (fgets(buffer, 1024, import_file) > 0) {
    buffer[strlen(buffer)-1] = '\0'; // remove new line from fgets

    Action_return result = cli_parse_input(buffer);
    switch (result.type) {
      case RETURN_SUCCESS: case RETURN_INFO:
        if (result.message && strcmp(result.message, ""))
          printf("Message from line %d (%s) of the import\n\n", line, buffer);
        break;

      case RETURN_ERROR:
        fclose(import_file);
        printf("Import failed at line %d (%s)\n", line, buffer);
        return ACTION_RETURN(RETURN_ERROR_AND_EXIT, "");

      case RETURN_ERROR_AND_EXIT:
        fclose(import_file);
        printf("Import failed miserably at line %d (%s)\n", line, buffer);
        return ACTION_RETURN(RETURN_ERROR_AND_EXIT, "");
    }

    line++;
  }

  fclose(import_file);
  return ACTION_RETURN(RETURN_SUCCESS, "");
}

Action_return do_nothing(Input *input) {
  input->cursor = input->length+1;
  return ACTION_RETURN(RETURN_SUCCESS, "");
}

Functionality cli_functionality[] = {
  { "" , "--", do_nothing }, // Comment and empty lines (for import)
  {"-l", "list", print_todo},
  { "" , "export", export_todo },
  { "" , "import", import_todo },
};

unsigned int cli_functionality_count() {
  return sizeof(cli_functionality) / sizeof(Functionality);
}

/// Parsing

Action_return cli_parse_input(char *input) {
  Input cmd = {
    .input = input,
    .length = strlen(input),
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
      break;
  }

  return action_return;
}
