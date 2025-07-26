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
  for (int i=1; i<argc; i++) {
    Action_return result;
    NESTED_ACTION(result = cli_parse_input(argv[i]), result);
    switch (result.type) {
      case RETURN_SUCCESS:
      case RETURN_INFO:
        if (result.message && strcmp(result.message, ""))
          PRINT_MESSAGE("Message from the %dº command (%s)", i, argv[i]);
        break;

      case RETURN_ERROR:
        PRINT_MESSAGE("An ERROR occurred in the %dº command (%s)", i, argv[i]);
        return false;

      case RETURN_ERROR_AND_EXIT:
        PRINT_MESSAGE("An ABORT occurred in the %dº command (%s)", i, argv[i]);
        return false;
    }
  }

  if (todo_list_modified) print_todo(NULL);
  return true;
}

int main(int argc, char *argv[]) {
  if (!lock_file()) return 1;
  load_file();

  int ret;
  if (argc == 1) {
    // TUI Version
    ret = window_app();
  } else {
    // CLI Version
    ret = (parse_argv_cli(argc, argv)) ? 0 : 1;
  }

  if (todo_list_modified) save_file();
  list_destroy(&todo_list, (void (*)(void *))free_todo);
  if (!unlock_file()) return 1;
  return ret;
}
