#ifndef DB_H
#define DB_H

#include <stdbool.h>
#include <stdio.h>
#include <time.h>
#include <stdint.h>

#include "utils/list.h"
#include "parser.h"

#define EXPORT_FILE_INDENTATION " │"

#define LOCK_FILENAME "lock"
#define LOCAL_PATH ".local/share/idea"
#define SAVE_FILENAME "todos.bin"
#define NOTES_TEMP_FILENAME "notes.md"

typedef struct {
  char *name; // Primary key

  char *hostname;
  uint64_t creation_time;
  char *notes;
} Todo;

extern List todo_list;
extern bool todo_list_modified;

Todo *create_todo(char *name);
bool todo_exists(const char *name);
bool search_todo_pos_by_name_or_pos(const char *name_or_position, unsigned int *index); // `position` should be 1-based. `index` is 0-based
void free_todo(Todo *node);

// DB
bool save_string_to_binary_file(FILE *file, char *str);
bool load_string_from_binary_file(FILE *file, char **str);
bool save_todo_to_binary_file(FILE *file, Todo *todo);
void *load_todo_from_binary_file(FILE *file);

// Import/ Export file
bool save_todo_to_export_file(FILE *file, Todo *todo);
bool load_todo_from_export_file(FILE *load_file, List *old_todo_list, bool *reached_eof);
bool write_notes_to_export_file(FILE *save_file, Todo *todo);

bool create_dir_if_not_exists(char *dir_path);
bool create_dir_structure();

bool load_todo_list();
bool save_todo_list();

void initialize_notes(Todo *todo);

Action_return action_add_todo(Input *input);
Action_return action_add_at_todo(Input * input);
Action_return action_clear_todos(Input * input);
Action_return action_remove_todo(Input *input);
Action_return action_move_todo(Input *input);
Action_return action_edit_todo(Input *input);
Action_return action_export_todo(Input *input);
Action_return action_import_todo(Input *input);
Action_return action_notes_todo_remove(Input *input);
Action_return action_generate_html(Input *input);
extern Functionality todo_list_functionality[];
extern unsigned int todo_list_functionality_count;

#endif
