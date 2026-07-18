#ifndef NOTES_PARSER_H
#define NOTES_PARSER_H

#include "todo_list.h"

typedef struct {
  Todo *todo;
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

typedef enum {
  ATTRIBUTE_TASK,
  ATTRIBUTE_REMINDER,
  ATTRIBUTE_TAG,
} Attribute_type;

// Tasks
bool is_task_incomplete(Task task);

// Properties: tags, reminders
// The properties are special keywords at the start of the line.

// Reminders
bool is_reminder_old(Reminder rem);
bool is_reminder_triggered(Reminder rem);
bool is_reminder_upcoming(Reminder rem);
bool is_reminder_near(Reminder rem);
Reminder *reminder_insertion_comparator(Reminder *rem1, Reminder *rem2);

// Attribute
bool build_attributes(Todo *todo);
void free_attributes(Todo *todo);

bool get_attributes_from_todo_list(List todos, Attribute_type attr_type, List *attributes); // TODO

#endif // NOTES_PARSER_H
