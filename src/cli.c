#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cli.h"
#include "parser.h"
#include "utils/list.h"

unsigned int msg_indentation = 1;

void print_todo(unsigned int index, Todo todo) {
  printf("%d) %s\n", index + 1, todo.data);
}

/// Functionality
Action_return action_print_todo(Input *input) {
  char *args;
  if ( input && (args = next_token(input, 0)) ) {
    free(args);
    return ACTION_RETURN(RETURN_ERROR, "list doesn't require arguments");
  }

  List_iterator iterator = list_iterator_create(todo_list);
  while (list_iterator_next(&iterator)) {
    const Todo *todo = list_iterator_element(iterator);
    print_todo(list_iterator_index(iterator), *todo);
  }
  return ACTION_RETURN(RETURN_SUCCESS, "");
}

void print_functionality(char *source, Functionality *functionality, unsigned int functionality_count) {
  printf(ANSI_UNDERLINE "%s" ANSI_RESET ":\n", source);
  if (functionality_count == 0) return;

  unsigned int max_cmd_length = 0;
  for (unsigned int i=0; i<functionality_count; i++) {
    const Functionality f = functionality[i];
    const unsigned int abbreviated_cmd_length = (f.abbreviation_cmd) ? strlen(f.abbreviation_cmd) : 0;
    const unsigned int full_cmd_length = strlen(f.full_cmd);
    const unsigned int cmd_length = abbreviated_cmd_length + full_cmd_length;
    if (cmd_length > max_cmd_length) max_cmd_length = cmd_length;
  }

  for (unsigned int i=0; i<functionality_count; i++) {
    const Functionality f = functionality[i];
    const unsigned int abbreviated_cmd_length = (f.abbreviation_cmd) ? strlen(f.abbreviation_cmd) : 0;
    const unsigned int full_cmd_length = strlen(f.full_cmd);
    const unsigned int cmd_length = abbreviated_cmd_length + full_cmd_length;

    const unsigned int description_padding = 3;
    unsigned int padding = max_cmd_length - cmd_length + description_padding;

    if (f.abbreviation_cmd) printf("\t" ANSI_YELLOW "%s" ANSI_RESET ", " ANSI_YELLOW "%s" ANSI_RESET "%*s%s\n", f.full_cmd, f.abbreviation_cmd, padding, " ", f.man.description);
    else                    printf("\t" ANSI_YELLOW "%s" ANSI_RESET "  %*s%s\n", f.full_cmd, padding, " ", f.man.description);

    const unsigned int separator_between_commands_length = 2; // ", " when f.abbreviation_cmd exist or "  " when it doesn't
    for (unsigned int x = 0; f.man.parameters[x]; x++) {
      unsigned int padding_parameter = max_cmd_length + description_padding + separator_between_commands_length;
      printf("\t%*sUsage: "ANSI_BLUE"%s %s"ANSI_RESET"\n", padding_parameter, "", f.full_cmd, functionality[i].man.parameters[x]);
    }
    printf("\n");
  }
}

Action_return action_print_help(Input *input) {
  char *args;
  if ( input && (args = next_token(input, 0)) ) {
    free(args);
    return ACTION_RETURN(RETURN_ERROR, "help doesn't require arguments");
  }

  print_functionality("Generic commands", todo_list_functionality, todo_list_functionality_count);
  print_functionality("CLI Specific commands", cli_functionality, cli_functionality_count);

  return ACTION_RETURN(RETURN_SUCCESS, "");
}

Action_return action_export_todo(Input *input) {
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

Action_return action_execute_commands(Input *input) {
  char *import_path = next_token(input, '\0');
  if (!import_path) return ACTION_RETURN(RETURN_ERROR, "Command malformed");

  FILE *import_file = fopen(import_path, "r");
  free(import_path);
  if (!import_file) return ACTION_RETURN(RETURN_ERROR, "Unable to open the import file");

  char buffer[1024];
  unsigned int line = 1;
  while (fgets(buffer, 1024, import_file)) {
    buffer[strlen(buffer)-1] = '\0'; // remove new line from fgets

    Action_return result;
    NESTED_ACTION(result = cli_parse_input(buffer), result);
    switch (result.type) {
      case RETURN_SUCCESS: case RETURN_INFO:
        if (result.message && strcmp(result.message, ""))
          PRINT_MESSAGE("Message from line %d (%s) of the commands in the file", line, buffer);
        break;

      case RETURN_ERROR:
        PRINT_MESSAGE("Execution of the commands in the file failed at line %d (%s)", line, buffer);
        fclose(import_file);
        return ACTION_RETURN(RETURN_ERROR_AND_EXIT, "Execution failed");

      case RETURN_ERROR_AND_EXIT:
        PRINT_MESSAGE("Execution of the commands in the file failed at line %d (%s)", line, buffer);
        fclose(import_file);
        return ACTION_RETURN(RETURN_ERROR_AND_EXIT, "Execution aborted");
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

Action_return action_import_todo(Input *input) {
  Action_return result = ACTION_RETURN(RETURN_SUCCESS, "");
  char command[TEMP_BUF_SIZE];
  char *base_path = get_path_from_variable("TMPDIR", "local_without_changes.idea");
  char *local_path = get_path_from_variable("TMPDIR", "local.idea");
  char *external_path = get_path_from_variable("TMPDIR", "external.idea");

  char *import_path = next_token(input, '\0');
  if (!import_path) {
    result = ACTION_RETURN(RETURN_ERROR, "Command malformed");
    goto exit;
  }

  // Clone the external file to a temporary location
  if (!clone_text_file(import_path, external_path)) {
    result = ACTION_RETURN(RETURN_ERROR, "Unable to copy the external file to the /tmp folder");
    goto exit;
  }

  // Generate the "local" file -that contains the actual database- (to diff it with the external file)
  snprintf(command, TEMP_BUF_SIZE, "export %s", local_path);
  Action_return function_return;
  NESTED_ACTION(function_return = cli_parse_input(command), function_return);
  switch (function_return.type) {
      case RETURN_SUCCESS: case RETURN_INFO:
        break;
      case RETURN_ERROR:
      case RETURN_ERROR_AND_EXIT:
        result = ACTION_RETURN(RETURN_ERROR_AND_EXIT, "Unable to export the local todos to diff them from the external one");
        goto exit;
  }
  // Clone the generated file (/tmp/local.idea) into (/tmp/local_without_changes.idea)
  // to compare the changes made with the diff tool (that are saved in /tmp/local.idea)
  // with the current state of the data base (/tmp/local_without_changes.idea)
  if(!clone_text_file(local_path, base_path)) {
    result = ACTION_RETURN(RETURN_ERROR, "Unable to clone the exported local database in the /tmp folder");
    goto exit;
  }

  // Execute the diff tool to get the changes the user wants.
  // The diff tool has to save the final version of the file
  // in /tmp/local.idea for idea to execute them.
  snprintf(command, TEMP_BUF_SIZE, DIFFTOOL_CMD " %s %s", local_path, external_path);
  int system_ret = system(command);
  if (system_ret == -1 || (WIFEXITED(system_ret) && WEXITSTATUS(system_ret) != 0)) {
    result = ACTION_RETURN(RETURN_ERROR, "Diff tool failed");
    goto exit;
  }

  // Show the user the changes in the commands that are going to be executed
  printf("Diff of the database commands:\n\n");
  snprintf(command, TEMP_BUF_SIZE, "%s %s %s", DIFF_CMD, base_path, local_path);
  system_ret = system(command);
  if (system_ret == -1 || (WIFEXITED(system_ret) && WEXITSTATUS(system_ret) == 2)) {
    result = ACTION_RETURN(RETURN_ERROR, "Final diff failed");
    goto exit;
  }
  if (WEXITSTATUS(system_ret) == 0) {
    result = ACTION_RETURN(RETURN_INFO, "The file to import is the same as the database. Import canceled");
    goto exit;
  }

  printf("\nDo you want execute this commands? [y/N] > ");
  char ans = getchar();
  if (ans != 'y' && ans != 'Y') {
    result = ACTION_RETURN(RETURN_INFO, "Import canceled");
    goto exit;
  }

  snprintf(command, TEMP_BUF_SIZE, "execute %s", local_path);
  NESTED_ACTION(function_return = cli_parse_input(command), function_return);
  switch (function_return.type) {
      case RETURN_SUCCESS: case RETURN_INFO:
        break;

      case RETURN_ERROR:
      case RETURN_ERROR_AND_EXIT:
        result = ACTION_RETURN(RETURN_ERROR_AND_EXIT, "Unable to apply the changes to the database");;
        goto exit;
  }

exit:
  remove(base_path);
  remove(local_path);
  remove(external_path);
  if (import_path) free(import_path);
  if (base_path) free(base_path);
  if (local_path) free(local_path);
  if (external_path) free(external_path);
  return result;
}

Action_return action_do_nothing(Input *input) {
  input->cursor = input->length+1;
  return ACTION_RETURN(RETURN_SUCCESS, "");
}

Functionality cli_functionality[] = {
  { "--", "", action_do_nothing, MAN("Comment. Mostly used in file exports/imports and executing files", "[text]") }, // Comment and empty lines
  { "list", "-l", action_print_todo, MAN("List all ToDos", NULL) },
  { "execute", NULL, action_execute_commands, MAN("Execute commands from a file", "[file]")},
  { "export", NULL, action_export_todo, MAN("Export the database", "[file]") },
  { "import", NULL, action_import_todo, MAN("Import a database generated by idea", "[file]") },
  { "help", "-h", action_print_help, MAN("Generic and CLI help page", NULL) },
};
unsigned int cli_functionality_count = sizeof(cli_functionality) / sizeof(Functionality);

/// Parsing

Action_return cli_parse_input(char *input) {
  Input cmd = {
    .input = input,
    .length = strlen(input),
    .cursor = 0,
  };
  char *instruction = next_token(&cmd, ' ');

  Action_return (*function)(Input *input) = search_functionality_function(instruction, cli_functionality, cli_functionality_count);
  if (!function) {
    function = search_functionality_function(instruction, todo_list_functionality, todo_list_functionality_count);
    todo_list_modified = true;
  }
  free(instruction);

  Action_return action_return = (function) ? function(&cmd) : ACTION_RETURN(RETURN_ERROR, "Invalid command");
  switch (action_return.type) {
    case RETURN_INFO:
    case RETURN_SUCCESS:
      if (cmd.cursor <= cmd.length) abort(); // Command parsing error. There's data left in the command
      break;

    case RETURN_ERROR:
    case RETURN_ERROR_AND_EXIT:
      todo_list_modified = false; // in order to not save the list
      break;
  }

  return action_return;
}

char *print_action_return(Action_return_type type) {
  switch (type) {
    case RETURN_SUCCESS: return "SUCCESS";
    case RETURN_INFO: return "INFO";
    case RETURN_ERROR: return "ERROR";
    case RETURN_ERROR_AND_EXIT: return "ABORT";
  }
  return NULL;
}
