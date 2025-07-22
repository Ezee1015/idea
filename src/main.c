#include "db.h"
#include "tui.h"
#include "cli.h"

int main(int argc, char *argv[]) {
  load_file();

  if (argc == 1) {
    // TUI Version
    window_app();
  } else {
    // CLI Version
    print_todo();
  }

  if (todo_list_modified) save_file();
  list_destroy(&todo_list, (void (*)(void *))free_todo);
  return 0;
}
