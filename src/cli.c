#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cli.h"
#include "parser.h"
#include "todo_list.h"
#include "utils/list.h"

/// Functionality
Action_return print_todo(Input *input) {
  (void) input;

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

Action_return execute_commands(Input *input) {
  char *import_path = next_token(input, '\0');
  if (!import_path) return ACTION_RETURN(RETURN_ERROR, "Command malformed");

  FILE *import_file = fopen(import_path, "r");
  free(import_path);
  if (!import_file) return ACTION_RETURN(RETURN_ERROR, "Unable to open the import file");

  char buffer[1024];
  unsigned int line = 1;
  while (fgets(buffer, 1024, import_file)) {
    buffer[strlen(buffer)-1] = '\0'; // remove new line from fgets

    Action_return result = cli_parse_input(buffer);
    switch (result.type) {
      case RETURN_SUCCESS: case RETURN_INFO:
        if (result.message && strcmp(result.message, ""))
          printf("Message from line %d (%s) of the commands in the file\n\n", line, buffer);
        break;

      case RETURN_ERROR:
        fclose(import_file);
        printf("Execution of the commands in the file failed at line %d (%s)\n", line, buffer);
        return ACTION_RETURN(RETURN_ERROR_AND_EXIT, "");

      case RETURN_ERROR_AND_EXIT:
        fclose(import_file);
        printf("Execution of the commands in the file failed miserably at line %d (%s)\n", line, buffer);
        return ACTION_RETURN(RETURN_ERROR_AND_EXIT, "");
    }

    line++;
  }

  fclose(import_file);
  return ACTION_RETURN(RETURN_SUCCESS, "");
}

bool clone_text_file(char *origin_path, char *clone_path) {
  FILE *origin = fopen(origin_path, "r");
  if (!origin) return false;
  FILE *clone = fopen(clone_path, "w");
  if (!clone) return false;

  int c; // int for EOF
  while ( (c = fgetc(origin)) != EOF ) fputc(c, clone);

  fclose(clone);
  fclose(origin);
  return true;
}

Action_return import_todo(Input *input) {
  char *import_path = next_token(input, '\0');
  if (!import_path) return ACTION_RETURN(RETURN_ERROR, "Command malformed");

  char *base_path =  get_path_from_variable("TMPDIR", "local_without_changes.idea");
  char *local_path = get_path_from_variable("TMPDIR", "local.idea");
  char *external_path = get_path_from_variable("TMPDIR", "external.idea");

  // Clone the external file to a temporary location
  if (!clone_text_file(import_path, external_path)) return ACTION_RETURN(RETURN_ERROR, "Unable to copy the external file to the /tmp folder");
  free(import_path);

  // Generate the "local" file -that contains the actual database- (to diff it with the external file)
  Input local_input = { .input = local_path, .length = strlen(local_path), .cursor = 0 };
  Action_return result = export_todo(&local_input);
  switch (result.type) {
    case RETURN_SUCCESS: case RETURN_INFO: break;
    case RETURN_ERROR: case RETURN_ERROR_AND_EXIT:
      printf("ERROR: Unable to export the local todos to diff them from the external one\n");
      return result;
  }
  // Clone the generated file (/tmp/local.idea) into (/tmp/local_without_changes.idea)
  // to compare the changes made with the diff tool (that are saved in /tmp/local.idea)
  // with the current state of the data base (/tmp/local_without_changes.idea)
  if(!clone_text_file(local_path, base_path)) return ACTION_RETURN(RETURN_ERROR, "Unable to clone the exported local database in the /tmp folder");

  // Execute the diff tool to get the changes the user wants.
  // The diff tool has to save the final version of the file
  // in /tmp/local.idea for idea to execute them.
  char command[TEMP_BUF_SIZE];
  snprintf(command, TEMP_BUF_SIZE, DIFF_TOOL" %s %s", local_path, external_path);
  int system_ret = system(command);
  if (system_ret == -1 || (WIFEXITED(system_ret) && WEXITSTATUS(system_ret) != 0)) {
    return ACTION_RETURN(RETURN_ERROR, "Diff tool failed");
  }

  // Show the user the changes in the commands that are going to be executed
  printf("Diff of the database commands:\n\n");
  // const char *diff_command = "diff --color";
  const char *diff_command = "diff --old-group-format="DIFF_FORMAT
                                 " --new-group-format="DIFF_FORMAT
                                 " --changed-group-format="DIFF_FORMAT
                                 " --unchanged-group-format='%='";
  snprintf(command, TEMP_BUF_SIZE, "%s %s %s", diff_command, base_path, local_path);
  system_ret = system(command);
  if (system_ret == -1 || (WIFEXITED(system_ret) && WEXITSTATUS(system_ret) == 2)) {
    return ACTION_RETURN(RETURN_ERROR, "Final diff failed");
  }
  if (WEXITSTATUS(system_ret) == 0) {
    return ACTION_RETURN(RETURN_INFO, "The file to import is the same as the database. Import canceled");
  }

  printf("\nDo you want execute this commands? [y/N] > ");
  char ans = getchar();
  if (ans != 'y' && ans != 'Y') {
    return ACTION_RETURN(RETURN_INFO, "Import canceled");
  }

  local_input.cursor = 0;
  result = execute_commands(&local_input);
  switch (result.type) {
    case RETURN_SUCCESS: case RETURN_INFO: break;
    case RETURN_ERROR: case RETURN_ERROR_AND_EXIT:
      printf("ERROR: Unable to apply the changes to the database\n");
      return result;
  }
  return ACTION_RETURN(RETURN_SUCCESS, "");
}

Action_return do_nothing(Input *input) {
  input->cursor = input->length+1;
  return ACTION_RETURN(RETURN_SUCCESS, "");
}

Functionality cli_functionality[] = {
  { "" , "--", do_nothing }, // Comment and empty lines (for import)
  {"-l", "list", print_todo},
  { "" , "execute", execute_commands },
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
