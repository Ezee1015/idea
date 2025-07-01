#include "db.h"
#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/// List

Todo_list *todo_list = {0};

Todo_list *get_node(unsigned int pos) {
  Todo_list *current = todo_list;
  while (--pos && current) current = current->next;
  if (pos != 0) return NULL;
  return current;
}

Todo_list *last_node() {
  Todo_list *current = todo_list;
  while (current->next) current = current->next;
  return current;
}

bool create_todo_node(Todo todo) {
  Todo_list *node = malloc(sizeof(Todo_list));
  if (!node) return false;
  node->todo = todo;
  node->next = NULL;

  if (!todo_list) {
    todo_list = node;
    return true;
  }

  last_node()->next = node;
  return true;
}

void free_todo(Todo todo) {
  free(todo.data);
}

void free_todo_node(Todo_list *node) {
  free_todo(node->todo);
  free(node);
}

Todo_list *remove_todo_node(unsigned int pos) {
  if (pos == 1) {
    Todo_list *aux = todo_list;
    todo_list = aux->next;
    return aux;
  }

  Todo_list *previous = get_node(pos - 1);
  if (!previous || !previous->next) return NULL; // Index out of bounds
  Todo_list *current = previous->next;
  previous->next = current->next;
  return current;
}

bool move_todo_node(unsigned int pos_origin, unsigned int pos_destination) {
  if (pos_origin == pos_destination) return true;

  Todo_list *node;
  if ( !(node = remove_todo_node(pos_origin)) ) return false;

  if (pos_origin < pos_destination) pos_destination--;

  if (!insert_node_at(node, pos_destination)) {
    insert_node_at(node, pos_origin);
    return false;
  } else {
    return true;
  }
}

bool insert_node_at(Todo_list *node, unsigned int pos) {
  if (!node) abort();

  if (pos == 1) {
    node->next = todo_list;
    todo_list = node;
    return true;
  }

  Todo_list *previous = get_node(pos-1);
  if (!previous) return false;
  node->next = previous->next;
  previous->next = node;
  return true;
}

/// FILE

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

bool save_todo(FILE *file, Todo todo) {
  bool ok = save_string(file, todo.data);
  return ok;
}

bool load_todo(FILE *file, Todo *todo) {
  if (!todo) abort();

  bool ok = load_string(file, &todo->data);
  return ok;
}

void save_file() {
  char *path = get_path(SAVE_PATH);
  FILE *save_file = fopen(path, "wb");
  free(path);
  if (!save_file) return;

  Todo_list *aux = todo_list;
  while (aux) {
    save_todo(save_file, aux->todo);
    aux = aux->next;
  }

  fclose(save_file);
  return;
}

void load_file() {
  if (todo_list) abort();

  char *path = get_path(SAVE_PATH);
  FILE *save_file = fopen(path, "rb");
  free(path);
  if (!save_file) return;

  Todo todo = {0};
  while (load_todo(save_file, &todo)) create_todo_node(todo);

  fclose(save_file);
  return;
}
