#ifndef DB_H
#define DB_H

#include <stdbool.h>
#include <stdio.h>
#include <time.h>
#include <stdint.h>

#include "utils/list.h"
#include "parser.h"

#define SAVE_FILE_INDENTATION " │"

#define LOCAL_PATH ".local/share/idea"

#define LOCK_FILENAME "lock"
#define SAVE_FILENAME "todos.txt"
#define NOTES_TEMP_FILENAME "notes.md"

#define UPCOMING_REMINDER_DAYS 10

typedef struct {
  char *name; // Primary key

  char *hostname;
  uint64_t creation_time;
  char *notes;
} Todo;

typedef struct {
  char *msg;
  char state;
  unsigned int level;
} Task;

typedef struct {
  Todo *todo;

  char *name;
  Date start;
  Date end;
} Reminder;

Todo *create_todo(char *name);
bool todo_exists(const char *name);
bool search_todo_pos_by_name_or_pos(const char *name_or_position, unsigned int *index); // `position` should be 1-based. `index` is 0-based
void free_todo(Todo *node);
bool is_a_valid_todo_name(char *name);

// Import/ Export file
bool save_todo_to_file(FILE *file, Todo *todo);
bool load_todo_from_file(const char *load_file_path, FILE *load_file, bool *reached_eof);
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

// Tasks (taken from the ToDo's notes)
void free_task(Task *task);
List get_tasks_from_todo(Todo todo);

// Get attribute from ToDo (taken from the ToDo's notes)
// The attributes are special keywords at the start of the line.
// Example: tags, reminder
List get_attribute_from_todo(Todo todo, const char *attribute, char argument_separator);

// Reminder (attribute)
bool parse_reminder(Todo *todo, char *rem_str, Reminder *rem);
bool is_reminder_old(Reminder rem);
bool is_reminder_triggered(Reminder rem);
bool is_reminder_upcoming(Reminder rem);
Reminder *reminder_insertion_comparator(Reminder *rem1, Reminder *rem2);
void free_reminder(Reminder *rem);
bool get_reminders_from_todo(Todo *todo, List *reminders);
bool get_all_reminders(List *reminders);

#endif
