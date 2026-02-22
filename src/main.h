#ifndef MAIN_H
#define MAIN_H

#include <stdbool.h>
#include <malloc.h>

#include "utils/list.h"
#include "utils/string.h"

typedef enum {
  BACKTRACE_ERROR,
  BACKTRACE_INFO,
} Backtrace_level;

typedef struct {
  char *message;
  const char *function_name;
  unsigned int line;
  char *file;
  Backtrace_level level;
} Backtrace_item;

extern List backtrace;

#define APPEND_TO_BACKTRACE(backtrace_level, ...) do { \
  Backtrace_item *b = malloc(sizeof(Backtrace_item));  \
  b->message = sb_create(__VA_ARGS__).str;             \
  b->level = backtrace_level;                          \
  b->function_name = __func__;                         \
  b->line = __LINE__;                                  \
  b->file = __FILE__;                                  \
  list_append(&backtrace, b);                          \
} while (0)

void free_backtrace_item(Backtrace_item *b);
// print_backtrace is implemented in each interface

typedef struct {
  char *local_path;
  char *tmp_path;
  char *lock_filepath;

  char *program_path;
} State;

extern State idea_state;

bool load_paths();
bool lock_file();
int main(int argc,char * argv[]);
bool parse_commands_cli(char * commands[],int count);
bool unlock_file();

#endif // MAIN_H
