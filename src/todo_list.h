#ifndef DB_H
#define DB_H

#include <stdbool.h>
#include <stdio.h>

#include "utils/list.h"
#include "parser.h"

#define CONFIG_PATH ".config/idea/"
#define NOTES_FOLDER "notes/"
#define SAVE_FILENAME "idea.bin"
#define NOTES_TEMPLATE "notes-XXXXXX.md"

typedef struct {
  char *data;
  char *notes_filename;
} Todo;

extern List todo_list;
extern bool todo_list_modified;

void free_todo(Todo *node);
bool export_template(FILE * file);
bool remove_todo_notes(Todo *todo);

// Save file
char *concatenate_paths(const char *directory, const char *relative);
char *get_path_from_variable(const char *variable, const char *path_from_directory);
bool save_string_to_binary_file(FILE *file, char *str);
bool load_string_from_binary_file(FILE *file, char **str);

bool save_todo_to_binary_file(FILE *file, Todo *todo);
void *load_todo_from_binary_file(FILE *file);
bool save_todo_to_text_file(FILE *file, Todo *todo);

bool create_dir_if_not_exists(char *dir_path);
bool create_dir_structure();

bool load_file();
bool save_file();

Action_return add_todo(Input *input);
Action_return remove_todo(Input *input);
Action_return move_todo(Input *input);
Action_return edit_todo(Input *input);
Action_return export_todo(Input *input);
Action_return import_todo(Input *input);
Action_return notes_todo_create(Input *input);
Action_return notes_todo_remove(Input *input);
extern Functionality todo_list_functionality[];
extern unsigned int todo_list_functionality_count;

#endif
