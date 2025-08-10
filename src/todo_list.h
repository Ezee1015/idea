#ifndef DB_H
#define DB_H

#include <stdbool.h>
#include <stdio.h>

#include "utils/list.h"
#include "parser.h"

#define CONFIG_PATH ".config/idea/"
#define EXPORT_FILE_INDENTATION " │"

#define NOTES_FOLDER "notes/"
#define SAVE_FILENAME "idea.bin"
#define NOTES_TEMPLATE "notes-XXXXXX.md"

#define str_starts_with(str, start) (!strncmp(start, str, strlen(start)))

typedef struct {
  char *id;
  char *name;
  bool notes;
} Todo;

extern List todo_list;
extern bool todo_list_modified;

unsigned int digit_count(long n);
char *generate_unique_todo_id();
void free_todo(Todo *node);
bool remove_todo_notes(Todo *todo);
Todo *create_todo(char *id);

// Save file
char *concatenate_paths(const char *directory, const char *relative);
char *get_path_from_variable(const char *variable, const char *path_from_directory);
char *read_line(FILE *f);
bool save_string_to_binary_file(FILE *file, char *str);
bool load_string_from_binary_file(FILE *file, char **str);

// DB
bool save_todo_to_binary_file(FILE *file, Todo *todo);
void *load_todo_from_binary_file(FILE *file);

// Import/ Export file
bool save_todo_to_text_file(FILE *file, Todo *todo);
bool load_todo_from_text_file(FILE *load_file, List *old_todo_list, bool *reached_eof);
bool write_notes_to_notes_file(FILE *file, char *notes);        // Export file --> Notes file
bool save_todo_notes_to_text_file(FILE *save_file, Todo *todo); // Notes file --> Export file

bool create_dir_if_not_exists(char *dir_path);
bool create_dir_structure();

bool load_file();
bool save_file();

bool create_notes_todo(Todo *todo);

Action_return action_add_todo(Input *input);
Action_return action_add_at_todo(Input * input);
Action_return action_clear_todos(Input * input);
Action_return action_remove_todo(Input *input);
Action_return action_move_todo(Input *input);
Action_return action_edit_todo(Input *input);
Action_return action_export_todo(Input *input);
Action_return action_import_todo(Input *input);
Action_return action_notes_todo_remove(Input *input);
extern Functionality todo_list_functionality[];
extern unsigned int todo_list_functionality_count;

#endif
