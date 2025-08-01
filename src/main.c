#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "todo_list.h"
#include "tui.h"
#include "cli.h"

bool lock_file() {
  char *lock_path = get_path_from_variable("TMPDIR", "idea.lock");
  if (access(lock_path, F_OK) == 0) {
    printf("Idea is already running...\n");
    free(lock_path);
    return false;
  }

  FILE *lock_file = fopen(lock_path, "w");
  if (!lock_file) {
    printf("Error creating the lock...\n");
    free(lock_path);
    return false;
  }

  free(lock_path);
  fclose(lock_file);
  return true;
}

bool unlock_file() {
  char *lock_path = get_path_from_variable("TMPDIR", "idea.lock");
  bool removed = (remove(lock_path) == 0);
  if (!removed) printf("Unable to remove the lock file\n");
  free(lock_path);
  return removed;
}

bool parse_argv_cli(int argc, char *argv[]) {
  if (!create_backup()) return false;

  int i=1;
  bool something_went_wrong = false;
  while (!something_went_wrong && i<argc) {
    Action_return result;
    NESTED_ACTION(result = cli_parse_input(argv[i]), result);
    switch (result.type) {
      case RETURN_SUCCESS:
      case RETURN_INFO:
        if (result.message && strcmp(result.message, ""))
          PRINT_MESSAGE("Message from the %dº command (%s)", i, argv[i]);
        break;

      case RETURN_ERROR:
        something_went_wrong = true;
        PRINT_MESSAGE("An ERROR occurred in the %dº command (%s)", i, argv[i]);
        break;

      case RETURN_ERROR_AND_EXIT:
        something_went_wrong = true;
        PRINT_MESSAGE("An ABORT occurred in the %dº command (%s)", i, argv[i]);
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

int main(int argc, char *argv[]) {
  if (!lock_file()) return 1;
  if (!load_file()) {
    unlock_file();
    return 1;
  }

  int ret;
  if (argc == 1) {
    // TUI Version
    ret = window_app();
  } else {
    // CLI Version
    ret = (parse_argv_cli(argc, argv)) ? 0 : 1;
  }

  if (todo_list_modified) {
    if (!save_file()) ret = 1;
  }
  list_destroy(&todo_list, (void (*)(void *))free_todo);
  if (!unlock_file()) return 1;
  return ret;
}
