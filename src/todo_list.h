#ifndef DB_H
#define DB_H

#include <stdbool.h>
#include <stdio.h>
#include <time.h>
#include <stdint.h>

#include "utils/list.h"
#include "parser.h"

#define SAVE_FILE_INDENTATION " │"

#define LOCK_FILENAME "lock"
#define LOCAL_PATH ".local/share/idea"
#define SAVE_FILENAME "todos.txt"
#define NOTES_TEMP_FILENAME "notes.md"

typedef struct {
  char *name; // Primary key

  char *hostname;
  uint64_t creation_time;
  char *notes;
} Todo;

Todo *create_todo(char *name);
bool todo_exists(const char *name);
bool search_todo_pos_by_name_or_pos(const char *name_or_position, unsigned int *index); // `position` should be 1-based. `index` is 0-based
void free_todo(Todo *node);
bool is_a_valid_todo_name(char *name);

// Import/ Export file
bool save_todo_to_file(FILE *file, Todo *todo);
bool load_todo_from_file(const char *load_file_path, FILE *load_file, List *old_todo_list, bool *reached_eof);
bool write_notes_to_file(FILE *save_file, Todo *todo);

bool create_dir_if_not_exists(char *dir_path);
bool create_dir_structure();

bool load_todo_list(List *list, char *file_path, bool obligatory);
bool save_todo_list(List list, char *file_path);

void initialize_notes(Todo *todo);

bool action_add_todo(Input *input);
bool action_add_at_todo(Input * input);
bool action_clear_todos(Input * input);
bool action_remove_todo(Input *input);
bool action_move_todo(Input *input);
bool action_edit_todo(Input *input);
bool action_export_todo(Input *input);
bool action_import_todo(Input *input);
bool action_notes_todo_remove(Input *input);
bool action_generate_html(Input *input);
extern Functionality todo_list_functionality[];
extern unsigned int todo_list_functionality_count;

#endif
