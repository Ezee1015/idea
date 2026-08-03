#ifndef MAIN_H
#define MAIN_H

#include "../utils/list.h"
#include "utils/config.h"

extern List todo_list;
extern bool todo_list_modified;

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

#endif // MAIN_H
