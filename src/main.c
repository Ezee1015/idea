#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "main.h"
#include "todo_list.h"
#include "tui.h"
#include "cli.h"
#include "utils/string.h"

State idea_state = {0};

bool lock_file() {
  if (access(idea_state.lock_filepath, F_OK) == 0) {
    printf("Idea is already running...\n");
    return false;
  }

  FILE *lock_file = fopen(idea_state.lock_filepath, "w");
  if (!lock_file) {
    printf("Error creating the lock...\n");
    return false;
  }

  fclose(lock_file);
  return true;
}

bool unlock_file() {
  bool removed = (remove(idea_state.lock_filepath) == 0);
  if (!removed) printf("Unable to remove the lock file\n");
  return removed;
}

bool parse_commands_cli(char *commands[], int count) {
  if (!create_backup()) return false;

  int i=0;
  bool something_went_wrong = false;
  while (!something_went_wrong && i<count) {
    Action_return result;
    NESTED_ACTION(result = cli_parse_input(commands[i]), result);
    switch (result.type) {
      case RETURN_SUCCESS:
      case RETURN_INFO:
        if (result.message && strcmp(result.message, ""))
          PRINT_MESSAGE("Message from the %dº command (%s)", i+1, commands[i]);
        break;

      case RETURN_ERROR:
        something_went_wrong = true;
        PRINT_MESSAGE("An ERROR occurred in the %dº command (%s)", i+1, commands[i]);
        break;

      case RETURN_ERROR_AND_EXIT:
        something_went_wrong = true;
        PRINT_MESSAGE("An ABORT occurred in the %dº command (%s)", i+1, commands[i]);
        break;
    }
    i++;
  }

  if (something_went_wrong) {
    PRINT_TEXT("Restoring the state from the Backup file...\n");
    if (!restore_backup()) {
      todo_list_modified = false; // Try to not save it because it may be corrupted
      PRINT_TEXT("Restoring the backup file failed... We're screwed...");
      PRINT_TEXT("Save right now a copy of the backup file (may be owerwritten in the future) and try to fix the problem");
      return false;
    }
  }

  remove_backup();
  if (todo_list_modified) action_print_todo(NULL);
  return !something_went_wrong;
}

bool load_paths() {
  String_builder sb = str_new();
  if (!str_append_from_shell_variable(&sb, "TMPDIR")) return false;
  idea_state.tmp_path = strdup(str_to_cstr(sb));

  str_append_to_path(&sb, BACKUP_NAME);
  idea_state.backup_filepath = strdup(str_to_cstr(sb));
  str_clean(&sb);

  str_append(&sb, idea_state.tmp_path);
  str_append_to_path(&sb, LOCK_FILENAME);
  idea_state.lock_filepath = strdup(str_to_cstr(sb));
  str_clean(&sb);

  if (!str_append_from_shell_variable(&sb, "HOME")) return false;
  str_append_to_path(&sb, CONFIG_PATH);
  idea_state.config_path = strdup(str_to_cstr(sb));

  str_append_to_path(&sb, NOTES_FOLDER);
  idea_state.notes_path = strdup(str_to_cstr(sb));
  str_free(&sb);

  return true;
}

void free_paths() {
  if (idea_state.tmp_path) free(idea_state.tmp_path);
  if (idea_state.backup_filepath) free(idea_state.backup_filepath);
  if (idea_state.lock_filepath) free(idea_state.lock_filepath);
  if (idea_state.config_path) free(idea_state.config_path);
  if (idea_state.notes_path) free(idea_state.notes_path);
}

int main(int argc, char *argv[]) {
  int ret;
  if (!load_paths()) { ret = 1; goto exit; }
  if (!lock_file())  { ret = 1; goto exit; }
  if (!load_file())  { ret = 1; goto exit; }

  if (argc == 1) {
    // TUI Version
    ret = window_app();
  } else {
    // CLI Version
    argv++; argc--;
    ret = (parse_commands_cli(argv, argc)) ? 0 : 1;
  }


exit:
  if (todo_list_modified && ret == 0) {
    if (!save_file()) ret = 1;
  }
  if (!unlock_file()) ret = 1;
  list_destroy(&todo_list, (void (*)(void *))free_todo);
  free_paths();
  return ret;
}
