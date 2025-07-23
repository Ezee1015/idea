#ifndef DB_H
#define DB_H

#include <stdbool.h>
#include <stdio.h>

#include "utils/list.h"
#include "parser.h"

#define SAVE_PATH ".local/share/idea"

typedef struct {
  // TODO
  char *data;
} Todo;

extern List todo_list;
extern bool todo_list_modified;

void free_todo(Todo *node);

// Save file
char *get_path(const char *path_from_home);
bool save_string(FILE *file, char *str);
bool load_string(FILE *file, char **str);

void *load_todo(FILE *file);
bool save_todo(FILE *file, Todo *todo);

void load_file();
void save_file();

Action_return add_todo(Input *cmd);
Action_return remove_todo(Input *cmd);
Action_return move_todo(Input *cmd);
Action_return edit_todo(Input *cmd);
extern Functionality todo_list_functionality[];
unsigned int todo_list_functionality_count();

#endif
