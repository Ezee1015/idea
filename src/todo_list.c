#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "utils/list.h"
#include "todo_list.h"

List todo_list = list_new();
bool todo_list_modified = false;

void free_todo(Todo *todo) {
  free(todo->data);
  free(todo);
}

/// FILE OPERATIONS
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

/// Functionality
Action_return add_todo(Input *input) {
  Todo *new = malloc(sizeof(Todo));
  if (!new) return ACTION_RETURN(RETURN_ERROR_AND_EXIT, "No more memory");
  new->data = next_token(input, 0);
  if (!new->data) {
    free(new);
    return ACTION_RETURN(RETURN_ERROR, "Command malformed");
  }
  list_append(&todo_list, new);
  return ACTION_RETURN(RETURN_SUCCESS, "");
}

Action_return remove_todo(Input *input) {
  char *pos_str = next_token(input, 0);
  if (!pos_str) return ACTION_RETURN(RETURN_ERROR, "Command malformed");
  unsigned int pos = atoi(pos_str);
  free(pos_str);

  if (pos == 0 || pos > list_size(todo_list)) return ACTION_RETURN(RETURN_ERROR, "Invalid Position");

  free_todo(list_remove(&todo_list, pos-1));
  return ACTION_RETURN(RETURN_SUCCESS, "");
}

Action_return move_todo(Input *input) {
  char *pos_origin_str = next_token(input, ' ');
  if (!pos_origin_str) return ACTION_RETURN(RETURN_ERROR, "Command malformed");
  unsigned int pos_origin = atoi(pos_origin_str);
  free(pos_origin_str);

  char *pos_destination_str = next_token(input, 0);
  if (!pos_destination_str) return ACTION_RETURN(RETURN_ERROR, "Command malformed");
  unsigned int pos_destination = atoi(pos_destination_str);
  free(pos_destination_str);

  if (pos_origin == 0 || pos_origin > list_size(todo_list)) return ACTION_RETURN(RETURN_ERROR, "Invalid origin position");
  if (pos_destination == 0 || pos_destination > list_size(todo_list)) return ACTION_RETURN(RETURN_ERROR, "Invalid destination position");
  if (pos_destination == pos_origin) return ACTION_RETURN(RETURN_INFO, "Moving to the same position");

  Todo *todo = list_remove(&todo_list, pos_origin-1);
  // if (pos_origin < pos_destination) pos_destination--;
  list_insert_at(&todo_list, todo, pos_destination-1);
  return ACTION_RETURN(RETURN_SUCCESS, "");
}

Action_return edit_todo(Input *input) {
    char *pos_str = next_token(input, ' ');
    if (!pos_str) return ACTION_RETURN(RETURN_ERROR, "Command malformed");
    unsigned int pos = atoi(pos_str);
    free(pos_str);

    if (pos == 0 || pos > list_size(todo_list)) return ACTION_RETURN(RETURN_ERROR, "Invalid Position");

    char *new_text = next_token(input, 0);
    if (!new_text) return ACTION_RETURN(RETURN_ERROR, "Empty new text.");

    Todo *todo = list_get(todo_list, pos-1);
    free(todo->data);
    todo->data = new_text;
    return ACTION_RETURN(RETURN_SUCCESS, "");
}

Functionality todo_list_functionality[] = {
  { "a", "add", add_todo },
  { "rm", "" , remove_todo },
  { "mv" , "move", move_todo },
  { "e" , "edit", edit_todo },
};

unsigned int todo_list_functionality_count() {
  return sizeof(todo_list_functionality) / sizeof(Functionality);
}
