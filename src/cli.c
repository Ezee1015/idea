#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cli.h"
#include "main.h"
#include "parser.h"
#include "todo_list.h"
#include "utils/list.h"
#include "utils/string.h"

bool cli_disable_colors = false;

void print_todo(unsigned int index, Todo todo) {
  printf( "%s%d)%s%s%s %s\n", ANSI_RED, index + 1, ANSI_GREEN, (todo.notes) ? " " NOTES_ICON : "", ANSI_RESET, todo.name);
}

bool clone_text_file(char *origin_path, char *clone_path) {
  FILE *origin = fopen(origin_path, "r");
  if (!origin) {
    APPEND_TO_BACKTRACE(BACKTRACE_ERROR, "Unable to open '%s'", origin_path);
    return false;
  }
  FILE *clone = fopen(clone_path, "w");
  if (!clone) {
    APPEND_TO_BACKTRACE(BACKTRACE_ERROR, "Unable to open '%s'", clone_path);
    return false;
  }

  int c; // int for EOF
  while ( (c = fgetc(origin)) != EOF ) fputc(c, clone);

  fclose(clone);
  fclose(origin);
  return true;
}

bool write_notes_to_temporal_file(Todo *todo) {
  String_builder notes_temp_path = sb_create("%s/" NOTES_TEMP_FILENAME, idea_state.local_path);
  FILE *f = fopen(notes_temp_path.str, "w");
  if (!f) {
    APPEND_TO_BACKTRACE(BACKTRACE_ERROR, "Unable to open '%s'", notes_temp_path.str);
    sb_free(&notes_temp_path);
    return false;
  }
  sb_free(&notes_temp_path);

  if (fputs(todo->notes, f) == EOF) {
    fclose(f);
    APPEND_TO_BACKTRACE(BACKTRACE_ERROR, "An error ocurred while writing to the note's temporal file");
    return false;
  }

  fclose(f);
  return true;
}

bool load_notes_from_temporal_file(Todo *todo) {
  String_builder notes_temp_path = sb_create("%s/" NOTES_TEMP_FILENAME, idea_state.local_path);
  FILE *f = fopen(notes_temp_path.str, "r");
  if (!f) {
    APPEND_TO_BACKTRACE(BACKTRACE_ERROR, "Unable to open '%s'", notes_temp_path.str);
    sb_free(&notes_temp_path);
    return false;
  }

  long size = 0;
  if ( fseek(f, 0, SEEK_END) == -1
       || (size = ftell(f)) == -1
       || fseek(f, 0, SEEK_SET) == -1) {
    APPEND_TO_BACKTRACE(BACKTRACE_ERROR, "Unable to get the length of the file '%s'", notes_temp_path.str);
    sb_free(&notes_temp_path);
    fclose(f);
    return false;
  }

  if (todo->notes) free(todo->notes);
  todo->notes = malloc(size+1);
  if (!todo->notes) return false;
  if (fread(todo->notes, size, 1, f) == 0) {
    APPEND_TO_BACKTRACE(BACKTRACE_ERROR, "Unable to read the file '%s'", notes_temp_path.str);
    sb_free(&notes_temp_path);
    fclose(f);
    return false;
  }
  todo->notes[size] = '\0';

  fclose(f);
  if (remove(notes_temp_path.str) == -1) APPEND_TO_BACKTRACE(BACKTRACE_ERROR, "Unable to remove the temporary notes file: %s", notes_temp_path.str);
  sb_free(&notes_temp_path);
  return true;
}

void cli_print_backtrace() {
  if (list_is_empty(backtrace)) return;

  const Backtrace_item *b = list_get(backtrace, 0);
  switch (b->level) {
    case BACKTRACE_INFO:  printf("\n" BOX_V_BAR " %s[INFO]%s\n"    BOX_V_BAR " %s", ANSI_BLUE, ANSI_RESET, ANSI_BLUE); break;
    case BACKTRACE_ERROR: printf("\n" BOX_V_BAR " %s[ERROR]%s\n"   BOX_V_BAR " %s", ANSI_RED,  ANSI_RESET, ANSI_RED ); break;
  }
  printf("%s%s\n" BOX_V_BAR, b->message, ANSI_RESET);

  printf("\n" BOX_V_BAR " %sBacktrace%s", ANSI_GRAY, ANSI_RESET);
  printf("\n" BOX_V_BAR " %s", ANSI_GRAY);
  for (int i=0; i<9; i++) if (i == 4) printf(BOX_T); else printf(BOX_H_BAR);
  printf("%s\n", ANSI_RESET);

  List_iterator iterator = list_iterator_create(backtrace);
  while (list_iterator_next(&iterator)) {
    const Backtrace_item *e = list_iterator_element(iterator);
    printf(BOX_V_BAR "     %s" BOX_V_BAR " %u) %s:%u:%s(): ", ANSI_GRAY, list_size(backtrace) - list_iterator_index(iterator), e->file, e->line, e->function_name);
    switch (b->level) {
      case BACKTRACE_INFO:  printf("[INFO]"); break;
      case BACKTRACE_ERROR: printf("[ERROR]"); break;
    }
    printf(" %s%s\n", e->message, ANSI_RESET);
  }
  printf("%s\n", ANSI_RESET);

  list_destroy(&backtrace, (void (*)(void *))free_backtrace_item);
}

/// Functionality
bool action_print_todo(Input *input) {
  ACTION_NO_ARGS("list", input);

  List_iterator iterator = list_iterator_create(todo_list);
  while (list_iterator_next(&iterator)) {
    const Todo *todo = list_iterator_element(iterator);
    print_todo(list_iterator_index(iterator), *todo);
  }
  return true;
}

void print_functionality(char *source, Functionality *functionality, unsigned int functionality_count) {
  printf("%s%s%s:\n", ANSI_UNDERLINE, source, ANSI_RESET);
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

    if (f.abbreviation_cmd) printf("\t%s%s%s, %s%s%s%*s%s\n", ANSI_YELLOW, f.full_cmd, ANSI_RESET, ANSI_YELLOW, f.abbreviation_cmd, ANSI_RESET, padding, " ", f.man.description);
    else                    printf("\t%s%s%s  %*s%s\n", ANSI_YELLOW, f.full_cmd, ANSI_RESET, padding, " ", f.man.description);

    const unsigned int separator_between_commands_length = 2; // ", " when f.abbreviation_cmd exist or "  " when it doesn't
    for (unsigned int x = 0; f.man.parameters[x]; x++) {
      unsigned int padding_parameter = max_cmd_length + description_padding + separator_between_commands_length;
      printf("\t%*s%sUsage: %s%s %s%s\n", padding_parameter, "", ANSI_GRAY, ANSI_BLUE, f.full_cmd, functionality[i].man.parameters[x], ANSI_RESET);
    }
    printf("\n");
  }
}

bool action_print_help(Input *input) {
  ACTION_NO_ARGS("help", input);

  printf("%sOpen TUI: %s%s\n", ANSI_GRAY, ANSI_RESET, idea_state.program_path);
  printf("%sCLI: %s%s [command 1] [command 2] [...]\n\n", ANSI_GRAY, ANSI_RESET, idea_state.program_path);
  printf("A simple ToDo-app written in C. Source code: %s<https://www.github.com/Ezee1015/idea>%s\n\n", ANSI_BLUE, ANSI_RESET);

  print_functionality("Generic commands", todo_list_functionality, todo_list_functionality_count);
  print_functionality("CLI Specific commands", cli_functionality, cli_functionality_count);

  return true;
}

bool action_export_todos(Input *input) {
  char *export_path = next_token(input, '\0');
  if (!export_path) {
    APPEND_TO_BACKTRACE(BACKTRACE_ERROR, "Command malformed: You must specify the export file path");
    return false;
  }

  String_builder path = sb_create("%s/" SAVE_FILENAME, idea_state.local_path);
  bool ret = save_todo_list(path.str);
  sb_free(&path);
  if (!ret) APPEND_TO_BACKTRACE(BACKTRACE_ERROR, "Unable to export all the ToDos");
  return ret;
}

bool action_execute_commands(Input *input) {
  char *import_path = next_token(input, '\0');
  if (!import_path) {
    APPEND_TO_BACKTRACE(BACKTRACE_ERROR, "Command malformed: You must specify the file path");
    return false;
  }

  FILE *cmds_file = fopen(import_path, "r");
  if (!cmds_file) {
    APPEND_TO_BACKTRACE(BACKTRACE_ERROR, "Unable to open the file '%s'", import_path);
    free(import_path);
    return false;
  }
  free(import_path); import_path = NULL;

  unsigned int line_nr = 1;
  String_builder line = sb_new();
  while ( (sb_read_line(cmds_file, &line)) ) {
    if (!line.length) {
      line_nr++;
      continue;
    }

    bool result;
    result = cli_parse_input(line.str);

    if (!result) {
      fclose(cmds_file);
      APPEND_TO_BACKTRACE(BACKTRACE_ERROR, "An error happened while executing the commands from %s:%u", line.str, line_nr);
      sb_free(&line);
      return false;
    }

    sb_clean(&line);
    line_nr++;
  }

  sb_free(&line);
  fclose(cmds_file);
  return true;
}

bool action_import_todos(Input *input) {
  char *import_path = next_token(input, '\0');
  if (!import_path) {
    APPEND_TO_BACKTRACE(BACKTRACE_ERROR, "Command malformed: You must specify the import file path");
    return false;
  }

  bool ok = load_todo_list(import_path, true);
  if (!ok) {
    APPEND_TO_BACKTRACE(BACKTRACE_ERROR, "Import of '%s' failed!", import_path);
    free(import_path);
    return false;
  }

  free(import_path);
  todo_list_modified = true;
  return true;
}

bool action_sync_todos(Input *input) {
  bool result = true;

  String_builder base_path     = sb_create("%s/local_without_changes.idea", idea_state.tmp_path);
  String_builder local_path    = sb_create("%s/local.idea", idea_state.tmp_path);
  String_builder external_path = sb_create("%s/external.idea", idea_state.tmp_path);

  char *import_path = next_token(input, '\0');
  if (!import_path) {
    APPEND_TO_BACKTRACE(BACKTRACE_ERROR, "Command malformed: You must specify the file path to sync");
    result = false;
    goto exit;
  }

  // Clone the external file to a temporary location
  if (!clone_text_file(import_path, external_path.str)) {
    APPEND_TO_BACKTRACE(BACKTRACE_ERROR, "Unable to copy the external file (%s) to %s", import_path, external_path.str);
    result = false;
    goto exit;
  }

  // Generate the "local" file -that contains the actual database- (to diff it with the external file)
  String_builder instruction = sb_create("export %s", local_path.str);

  bool function_return;
  function_return = cli_parse_input(instruction.str);
  sb_free(&instruction);
  if (!function_return) {
    APPEND_TO_BACKTRACE(BACKTRACE_ERROR, "Unable to export the local ToDos to diff them");
    result = false;
    goto exit;
  }
  // Clone the generated file (/tmp/local.idea) into (/tmp/local_without_changes.idea)
  // to compare the changes made with the diff tool (that are saved in /tmp/local.idea)
  // with the current state of the data base (/tmp/local_without_changes.idea)
  if (!clone_text_file(local_path.str, base_path.str)) {
    APPEND_TO_BACKTRACE(BACKTRACE_ERROR, "Unable to clone the exported local database (%s) to '%s'", local_path.str, base_path.str);
    result = false;
    goto exit;
  }

  // Execute the diff tool to get the changes the user wants.
  // The diff tool has to save the final version of the file
  // in /tmp/local.idea for idea to execute them.
  instruction = sb_create("%s %s %s", DIFFTOOL_CMD, local_path.str, external_path.str);
  int system_ret = system(instruction.str);
  sb_free(&instruction);
  if (system_ret == -1 || (WIFEXITED(system_ret) && WEXITSTATUS(system_ret) != 0)) {
    APPEND_TO_BACKTRACE(BACKTRACE_ERROR, "Diff tool failed");
    result = false;
    goto exit;
  }

  // Show the user the changes in the commands that are going to be executed
  printf("Diff of the database commands:\n\n");
  instruction = sb_create(DIFF_CMD_FMT " %s %s", DIFF_CMD_ARGS, base_path.str, local_path.str);
  system_ret = system(instruction.str);
  sb_free(&instruction);
  if (system_ret == -1 || (WIFEXITED(system_ret) && WEXITSTATUS(system_ret) == 2)) {
    APPEND_TO_BACKTRACE(BACKTRACE_ERROR, "Final diff failed");
    result = false;
    goto exit;
  }
  if (WEXITSTATUS(system_ret) == 0) {
    APPEND_TO_BACKTRACE(BACKTRACE_INFO, "The file to sync is the same as the database. Sync canceled!");
    result = true;
    goto exit;
  }

  printf("\nDo you want to import this ToDos? The current database will be deleted [y/N] > ");
  char ans = getchar();
  if (ans != 'y' && ans != 'Y') {
    APPEND_TO_BACKTRACE(BACKTRACE_INFO, "Import canceled");
    result = true;
    goto exit;
  }

  if (!load_todo_list(local_path.str, true)) {
    APPEND_TO_BACKTRACE(BACKTRACE_ERROR, "Import failed");
    result = false;
    goto exit;
  }
  todo_list_modified = true;

exit:
  remove(base_path.str);
  remove(local_path.str);
  remove(external_path.str);
  sb_free(&base_path);
  sb_free(&local_path);
  sb_free(&external_path);

  if (import_path) free(import_path);
  return result;
}

bool action_notes_todo(Input *input) {
  char *arg = next_token(input, 0);
  if (!arg) {
    APPEND_TO_BACKTRACE(BACKTRACE_ERROR, "Command malformed: you must specify the ToDo");
    return false;
  }

  unsigned int pos;
  if (!search_todo_pos_by_name_or_pos(arg, &pos)) {
    APPEND_TO_BACKTRACE(BACKTRACE_ERROR, "Unable to find the ToDo '%s'", arg);
    free(arg);
    return false;
  }
  free(arg); arg = NULL;

  Todo *todo = list_get(todo_list, pos);

  if (!todo->notes) initialize_notes(todo);

  if (!write_notes_to_temporal_file(todo)) {
    APPEND_TO_BACKTRACE(BACKTRACE_ERROR, "Unable to write the notes to the temporary file");
    return false;
  }

  String_builder instruction = sb_create("%s '%s/" NOTES_TEMP_FILENAME "'", TEXT_EDITOR, idea_state.local_path);
  int system_ret = system(instruction.str);
  sb_free(&instruction);
  if (system_ret == -1 || (WIFEXITED(system_ret) && WEXITSTATUS(system_ret) != 0)) {
    APPEND_TO_BACKTRACE(BACKTRACE_ERROR, "The text editor exited with error code %d", WEXITSTATUS(system_ret));
    return false;
  }

  if (!load_notes_from_temporal_file(todo)) {
    APPEND_TO_BACKTRACE(BACKTRACE_ERROR, "Unable to read the notes from the temporary file");
    return false;
  }

  todo_list_modified = true;
  return true;
}

#ifdef COMMIT
bool action_version(Input *input) {
  ACTION_NO_ARGS("version", input);

  printf("%sVersion: %s" COMMIT "\n", ANSI_GRAY, ANSI_RESET);
  printf("%sLocal path: %s%s\n", ANSI_GRAY, ANSI_RESET, idea_state.local_path);
  return true;
}
#endif // COMMIT

bool action_do_nothing(Input *input) {
  input->cursor = input->length+1;
  return true;
}

Functionality cli_functionality[] = {
  { "--", "", action_do_nothing, MAN("Comment. Mostly used in file exports/imports and executing files", "[text]") }, // Comment and empty lines
  { "list", "-l", action_print_todo, MAN("List all ToDos", NULL) },
  { "execute", NULL, action_execute_commands, MAN("Execute a list of idea commands from a text file", "[file]")},
  { "export", NULL, action_export_todos, MAN("Export the ToDos to a text file", "[file]") },
  { "sync", NULL, action_sync_todos, MAN("Check and import the ToDos from a text file generated by idea", "[file]") },
  { "import", NULL, action_import_todos, MAN("Import the ToDos without any interaction (no diff)", "[file]") },
  { "help", "-h", action_print_help, MAN("Help page", NULL) },
  { "notes", NULL, action_notes_todo, MAN("Open the ToDo's notes file", "[index]", "[name]") },
#ifdef COMMIT
  { "version", "-v", action_version, MAN("Print the commit hash and version", NULL) },
#endif // COMMIT
};
unsigned int cli_functionality_count = sizeof(cli_functionality) / sizeof(Functionality);

/// Parsing

bool cli_parse_input(char *input) {
  Input cmd = {
    .input = input,
    .length = strlen(input),
    .cursor = 0,
  };
  char *instruction = next_token(&cmd, ' ');

  bool (*function)(Input *input) = search_functionality_function(instruction, cli_functionality, cli_functionality_count);
  if (!function) {
    function = search_functionality_function(instruction, todo_list_functionality, todo_list_functionality_count);
  }

  if (!function) {
    APPEND_TO_BACKTRACE(BACKTRACE_ERROR, "Unknown command '%s'", instruction);
    free(instruction);
    return false;
  }

  if (function(&cmd)) {
    if (cmd.cursor <= cmd.length) {
      APPEND_TO_BACKTRACE(BACKTRACE_ERROR, "Command parsing error. There's data left in the command parameters. Command: '%s'", instruction);
    }
    free(instruction);
    return true;
  } else {
    APPEND_TO_BACKTRACE(BACKTRACE_ERROR, "Command '%s' failed", instruction);
    free(instruction);
    return false;
  }
}
