#ifndef DB_H
#define DB_H

#include <stdbool.h>
#include <stdio.h>

#define SAVE_PATH ".local/share/idea"

typedef struct {
  // TODO
  char *data;
} Todo;

struct Todo_list {
  Todo todo;

  struct Todo_list *next;
};
typedef struct Todo_list Todo_list;

extern Todo_list *todo_list;

// Todo_list
Todo_list *get_node(unsigned int pos);
bool create_todo_node(Todo todo);
void free_todo_node(Todo_list *node);
void free_todo(Todo todo);
Todo_list *remove_todo_node(unsigned int pos);
bool move_todo_node(unsigned int pos_origin, unsigned int pos_destination);
bool insert_node_at(Todo_list *node, unsigned int pos);

// Save file
char *get_path(const char *path_from_home);
bool save_string(FILE *file, char *str);
bool load_string(FILE *file, char **str);

bool load_todo(FILE *file, Todo *todo);
bool save_todo(FILE *file, Todo todo);
void load_file();
void save_file();

#endif
