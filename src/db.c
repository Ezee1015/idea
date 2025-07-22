#include "db.h"
#include "utils/list.h"
#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "utils/list.h"

List todo_list = list_new();

void free_todo(Todo *todo) {
  free(todo->data);
  free(todo);
}

char *get_path(const char *path_from_home) {
    const char *home = getenv("HOME");
    if (!home) return NULL;

    unsigned int home_length = strlen(home);
    unsigned int len_rel = strlen(path_from_home);

    char *full_path = malloc(home_length + 1 + len_rel + 1);
    if (!full_path) return NULL;
    sprintf(full_path, "%s/%s", home, path_from_home);
    return full_path;
}

bool save_string(FILE *file, char *str) {
  unsigned int size = strlen(str);
  if (fwrite(&size, sizeof(unsigned int), 1, file) != 1) return false;
  if (fwrite(str, size, 1, file) != 1) return false;
  return true;
}

bool load_string(FILE *file, char **str) {
  unsigned int size;
  if (fread(&size, sizeof(unsigned int), 1, file) != 1) return false;
  *str = malloc(size+1);
  if (fread(*str, size, 1, file) != 1) return false;
  (*str)[size] = '\0';
  return true;
}

bool save_todo(FILE *file, Todo *todo) {
  if (!save_string(file, todo->data)) return false;
  return true;
}

void *load_todo(FILE *file) {
  Todo *todo = malloc(sizeof(Todo));
  if (!todo) abort();
  if (!load_string(file, &todo->data)) return false;
  return todo;
}

void save_file() {
  char *path = get_path(SAVE_PATH);
  FILE *save_file = fopen(path, "wb");
  free(path);
  if (!save_file) return;

  list_save_to_bfile(todo_list, (bool (*)(FILE *, void *)) save_todo, save_file);

  fclose(save_file);
  return;
}

void load_file() {
  if (!list_is_empty(todo_list)) abort();
  char *path = get_path(SAVE_PATH);
  FILE *save_file = fopen(path, "rb");
  free(path);
  if (!save_file) return;

  list_load_from_bfile(&todo_list, load_todo, save_file);

  fclose(save_file);
  return;
}
