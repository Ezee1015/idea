#ifndef DB_H
#define DB_H

#include <stdbool.h>
#include <stdio.h>
#include "utils/list.h"

#define SAVE_PATH ".local/share/idea"

typedef struct {
  // TODO
  char *data;
} Todo;

extern List todo_list;

void free_todo(Todo *node);

// Save file
char *get_path(const char *path_from_home);
bool save_string(FILE *file, char *str);
bool load_string(FILE *file, char **str);

void *load_todo(FILE *file);
bool save_todo(FILE *file, Todo *todo);

void load_file();
void save_file();

#endif
