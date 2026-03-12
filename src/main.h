#ifndef MAIN_H
#define MAIN_H

#include <stdbool.h>
#include <malloc.h>

#include "utils/list.h"
#include "utils/string.h"
#include "config.h"

extern List todo_list;
extern bool todo_list_modified;

// Backtrace

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

void free_backtrace_item(Backtrace_item *b);
// print_backtrace is implemented in each interface

#define APPEND_TO_BACKTRACE(backtrace_level, ...) do { \
  Backtrace_item *b = malloc(sizeof(Backtrace_item));  \
  b->message = sb_create(__VA_ARGS__).str;             \
  b->level = backtrace_level;                          \
  b->function_name = __func__;                         \
  b->line = __LINE__;                                  \
  b->file = __FILE__;                                  \
  list_append(&backtrace, b);                          \
} while (0)

// Date

typedef struct {
  int year;
  int month;
  int day;
} Date;

Date date_now();
int get_delta_time_days(Date date_from, Date date_to);
char *get_delta_time_string(Date date_from, Date date_to);
bool load_date_from_string(char *date_str, Date *date);

bool is_date_equals(Date date_1, Date date_2);
bool is_date_greater(Date date_greater, Date date_less);
bool is_date_less(Date date_less, Date date_greater);
#define is_date_less_or_equals(date_1, date_2) (is_date_less(date_1, date_2) || is_date_equals(date_1, date_2))
#define is_date_greater_or_equals(date_1, date_2) (is_date_greater(date_1, date_2) || is_date_equals(date_1, date_2))

/////////////

typedef struct {
  char *local_path;
  char *tmp_path;
  char *lock_filepath;
  char *todos_filepath;
  char *config_filepath;

  Config config;

  char *program_path;
} State;

extern State idea_state;

bool load_paths();
bool lock_file();
int main(int argc,char * argv[]);
bool parse_commands_cli(char * commands[],int count);
bool unlock_file();

#endif // MAIN_H
