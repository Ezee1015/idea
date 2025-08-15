#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cli.h"
#include "main.h"
#include "parser.h"
#include "todo_list.h"
#include "utils/list.h"
#include "utils/string.h"

unsigned int msg_indentation = 1;

void print_todo(unsigned int index, Todo todo) {
  printf(ANSI_RED "%d)" ANSI_GREEN "%s" ANSI_RESET " %s\n", index + 1, (todo.notes) ? " " NOTES_ICON : "", todo.name);
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

bool create_backup() {
  String_builder instruction = str_new();
  str_append(&instruction, "export ");
  str_append(&instruction, idea_state.backup_filepath);

  Action_return result;
  NESTED_ACTION(result = cli_parse_input(str_to_cstr(instruction)), result);
  bool ok;
  switch (result.type) {
    case RETURN_SUCCESS: case RETURN_INFO:
      if (result.message && strcmp(result.message, ""))
        PRINT_MESSAGE("Message from the backup file (%s)...", idea_state.backup_filepath);
      ok = true;
      break;

    case RETURN_ERROR:
    case RETURN_ERROR_AND_EXIT:
      PRINT_MESSAGE("Unable to create the backup file (%s)...", idea_state.backup_filepath);
      ok = false;
      break;
  }

  str_free(&instruction);
  return ok;
}

bool restore_backup() {
  String_builder instruction = str_new();
  str_append(&instruction, "import_no_diff ");
  str_append(&instruction, idea_state.backup_filepath);

  Action_return result;
  NESTED_ACTION(result = cli_parse_input(str_to_cstr(instruction)), result);
  bool ok;
  switch (result.type) {
    case RETURN_SUCCESS: case RETURN_INFO:
      if (result.message && strcmp(result.message, ""))
        PRINT_MESSAGE("Message from the backup file (%s)", idea_state.backup_filepath);
      ok = true;
      break;

    case RETURN_ERROR:
    case RETURN_ERROR_AND_EXIT:
      PRINT_MESSAGE("Unable to restore from the backup file... The backup file is located *TEMPORARY* at %s", idea_state.backup_filepath);
      ok = false;
      break;
  }

  str_free(&instruction);
  return ok;
}

void remove_backup() {
  remove(idea_state.backup_filepath);
}

bool import_file(char *filepath) {
  FILE *file = fopen(filepath, "r");
  if (!file) return false;

  List old_todo_list = todo_list;
  todo_list = list_new();

  bool reached_eof = false;
  while (load_todo_from_text_file(file, &old_todo_list, &reached_eof));
  fclose(file);
  if (!list_map_bool(old_todo_list, (bool (*)(void *))remove_todo_notes)) return false;
  list_destroy(&old_todo_list, (void (*)(void *))free_todo);
  return reached_eof;
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
  char *export_path = next_token(input, '\0');
  if (!export_path) return ACTION_RETURN(RETURN_ERROR, "Command malformed");

  FILE *export_file = fopen(export_path, "w");
  free(export_path); export_path = NULL;
  if (!export_file) return ACTION_RETURN(RETURN_ERROR, "Unable to create the export file");

  if (fprintf(export_file, "-- File generated by idea. Edit this file with caution.\n\n") <= 0) {
    fclose(export_file);
    return ACTION_RETURN(RETURN_ERROR, "Unable to save the export file header");
  }
  List_iterator iterator = list_iterator_create(todo_list);
  while (list_iterator_next(&iterator)) {
    if (!save_todo_to_text_file(export_file, list_iterator_element(iterator))) {
      fclose(export_file);
      return ACTION_RETURN(RETURN_ERROR, "Unable to save todo");
    }
    if (list_iterator_has_next(iterator) && fprintf(export_file, "\n") <= 0) {
      fclose(export_file);
      return ACTION_RETURN(RETURN_ERROR, "Unable to print new line in the file");
    }
  }

  fclose(export_file);
  return ACTION_RETURN(RETURN_SUCCESS, "");
}

Action_return action_execute_commands(Input *input) {
  char *import_path = next_token(input, '\0');
  if (!import_path) return ACTION_RETURN(RETURN_ERROR, "Command malformed");

  FILE *cmds_file = fopen(import_path, "r");
  free(import_path); import_path = NULL;
  if (!cmds_file) return ACTION_RETURN(RETURN_ERROR, "Unable to open the import file");

  unsigned int line = 1;
  char *str;
  while ( (str = read_line(cmds_file)) ) {
    Action_return result;
    NESTED_ACTION(result = cli_parse_input(str), result);
    switch (result.type) {
      case RETURN_SUCCESS: case RETURN_INFO:
        if (result.message && strcmp(result.message, ""))
          PRINT_MESSAGE("Message from line %d (%s) of the commands in the file", line, str);
        free(str);
        break;

      case RETURN_ERROR:
        PRINT_MESSAGE("Execution of the commands in the file failed at line %d (%s)", line, str);
        fclose(cmds_file);
        free(str);
        return ACTION_RETURN(RETURN_ERROR_AND_EXIT, "Execution failed");

      case RETURN_ERROR_AND_EXIT:
        PRINT_MESSAGE("Execution of the commands in the file failed at line %d (%s)", line, str);
        fclose(cmds_file);
        free(str);
        return ACTION_RETURN(RETURN_ERROR_AND_EXIT, "Execution aborted");
    }

    line++;
  }

  fclose(cmds_file);
  return ACTION_RETURN(RETURN_SUCCESS, "");
}

Action_return action_import_todo_no_diff(Input *input) {
  char *import_path = next_token(input, '\0');
  if (!import_path) {
    return ACTION_RETURN(RETURN_ERROR, "Command malformed");
  }

  bool ok = import_file(import_path);
  free(import_path); import_path = NULL;
  if (!ok) return ACTION_RETURN(RETURN_ERROR, "Import failed!");

  todo_list_modified = true;
  return ACTION_RETURN(RETURN_SUCCESS, "");
}

Action_return action_import_todo(Input *input) {
  Action_return result = ACTION_RETURN(RETURN_SUCCESS, "");

  String_builder base_path = str_new();
  str_append(&base_path, idea_state.tmp_path);
  str_append_to_path(&base_path, "local_without_changes.idea");

  String_builder local_path = str_new();
  str_append(&local_path, idea_state.tmp_path);
  str_append_to_path(&local_path, "local.idea");

  String_builder external_path = str_new();
  str_append(&external_path, idea_state.tmp_path);
  str_append_to_path(&external_path, "external.idea");

  char *import_path = next_token(input, '\0');
  if (!import_path) {
    result = ACTION_RETURN(RETURN_ERROR, "Command malformed");
    goto exit;
  }

  // Clone the external file to a temporary location
  if (!clone_text_file(import_path, str_to_cstr(external_path))) {
    result = ACTION_RETURN(RETURN_ERROR, "Unable to copy the external file to the /tmp folder");
    goto exit;
  }

  // Generate the "local" file -that contains the actual database- (to diff it with the external file)
  String_builder instruction = str_new();
  str_append(&instruction, "export ");
  str_append_str(&instruction, local_path);

  Action_return function_return;
  NESTED_ACTION(function_return = cli_parse_input(str_to_cstr(instruction)), function_return);
  str_free(&instruction);
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
  if (!clone_text_file(str_to_cstr(local_path), str_to_cstr(base_path))) {
    result = ACTION_RETURN(RETURN_ERROR, "Unable to clone the exported local database in the /tmp folder");
    goto exit;
  }

  // Execute the diff tool to get the changes the user wants.
  // The diff tool has to save the final version of the file
  // in /tmp/local.idea for idea to execute them.
  str_append(&instruction, DIFFTOOL_CMD " ");
  str_append_str(&instruction, local_path);
  str_append(&instruction, " ");
  str_append_str(&instruction, external_path);
  int system_ret = system(str_to_cstr(instruction));
  str_free(&instruction);
  if (system_ret == -1 || (WIFEXITED(system_ret) && WEXITSTATUS(system_ret) != 0)) {
    result = ACTION_RETURN(RETURN_ERROR, "Diff tool failed");
    goto exit;
  }

  // Show the user the changes in the commands that are going to be executed
  printf("Diff of the database commands:\n\n");
  str_append(&instruction, DIFF_CMD " ");
  str_append_str(&instruction, base_path);
  str_append(&instruction, " ");
  str_append_str(&instruction, local_path);
  system_ret = system(str_to_cstr(instruction));
  str_free(&instruction);
  if (system_ret == -1 || (WIFEXITED(system_ret) && WEXITSTATUS(system_ret) == 2)) {
    result = ACTION_RETURN(RETURN_ERROR, "Final diff failed");
    goto exit;
  }
  if (WEXITSTATUS(system_ret) == 0) {
    result = ACTION_RETURN(RETURN_INFO, "The file to import is the same as the database. Import canceled");
    goto exit;
  }

  printf("\nDo you want to import this ToDos? The current database will be deleted [y/N] > ");
  char ans = getchar();
  if (ans != 'y' && ans != 'Y') {
    result = ACTION_RETURN(RETURN_INFO, "Import canceled");
    goto exit;
  }

  if (!import_file(str_to_cstr(local_path))) {
    result = ACTION_RETURN(RETURN_ERROR, "Import failed!");
    goto exit;
  }
  todo_list_modified = true;

exit:
  remove(str_to_cstr(base_path));
  remove(str_to_cstr(local_path));
  remove(str_to_cstr(external_path));
  str_free(&base_path);
  str_free(&local_path);
  str_free(&external_path);

  if (import_path) free(import_path);
  return result;
}

Action_return notes_todo(Input *input) {
  char *pos_str = next_token(input, 0);
  if (!pos_str) return ACTION_RETURN(RETURN_ERROR, "Command malformed");
  unsigned int pos = atoi(pos_str);
  free(pos_str); pos_str = NULL;
  if (!pos) return ACTION_RETURN(RETURN_ERROR, "The given position is not a number");
  if (pos == 0 || pos > list_size(todo_list)) return ACTION_RETURN(RETURN_ERROR, "Invalid position");
  Todo *todo = list_get(todo_list, pos-1);

  if (!todo->notes) {
    if (!create_notes_todo(todo)) return ACTION_RETURN(RETURN_ERROR, "Unable to create the notes file");
    todo_list_modified = true;
  }

  String_builder instruction = str_new();
  str_append(&instruction, TEXT_EDITOR " '");
  str_append(&instruction, idea_state.notes_path);
  str_append_to_path(&instruction, todo->id);
  str_append(&instruction, ".md'");

  int system_ret = system(str_to_cstr(instruction));
  str_free(&instruction);
  if (system_ret == -1 || (WIFEXITED(system_ret) && WEXITSTATUS(system_ret) != 0)) {
    return ACTION_RETURN(RETURN_ERROR, "Text editor failed or file doesn't exist");
  }

  return ACTION_RETURN(RETURN_SUCCESS, "");
}

#ifdef COMMIT
Action_return action_version(Input *input) {
  char *args;
  if ( input && (args = next_token(input, 0)) ) {
    free(args);
    return ACTION_RETURN(RETURN_ERROR, "version doesn't require arguments");
  }

  printf(ANSI_GRAY "Version: " ANSI_RESET COMMIT "\n");
  printf(ANSI_GRAY "Config: "ANSI_RESET "%s\n", idea_state.config_path);
  return ACTION_RETURN(RETURN_SUCCESS, "");
}
#endif // COMMIT

Action_return action_do_nothing(Input *input) {
  input->cursor = input->length+1;
  return ACTION_RETURN(RETURN_SUCCESS, "");
}

Functionality cli_functionality[] = {
  { "--", "", action_do_nothing, MAN("Comment. Mostly used in file exports/imports and executing files", "[text]") }, // Comment and empty lines
  { "list", "-l", action_print_todo, MAN("List all ToDos", NULL) },
  { "execute", NULL, action_execute_commands, MAN("Execute a list of idea commands from a text file", "[file]")},
  { "export", NULL, action_export_todo, MAN("Export the ToDos to a text file", "[file]") },
  { "import", NULL, action_import_todo, MAN("Import the ToDos from a text file generated by idea. This will replace the current database", "[file]") },
  { "import_no_diff", NULL, action_import_todo_no_diff, MAN("Import the ToDos without any interaction (no diff)", "[file]") },
  { "help", "-h", action_print_help, MAN("Help page", NULL) },
  { "notes", NULL, notes_todo, MAN("Open the ToDo's notes file", "[ID]") },
#ifdef COMMIT
  { "version", "-v", action_version, MAN("Print the commit hash and version", NULL) },
#endif // COMMIT
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
  free(instruction); instruction = NULL;

  Action_return action_return = (function) ? function(&cmd) : ACTION_RETURN(RETURN_ERROR, "Invalid command");
  switch (action_return.type) {
    case RETURN_INFO:
    case RETURN_SUCCESS:
      if (cmd.cursor <= cmd.length) abort(); // Command parsing error. There's data left in the command
      break;

    case RETURN_ERROR:
    case RETURN_ERROR_AND_EXIT:
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
