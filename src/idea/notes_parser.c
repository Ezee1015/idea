#include "notes_parser.h"
#include <string.h>

void free_task(Task *task) {
  free(task->msg);
  free(task);
}

bool is_a_task(const char *cstr, unsigned int length) {
  if (!cstr || *cstr == '\0') return false;

  unsigned int i=0;

  bool bullet = (cstr[i] == '-' || cstr[i] == '+' || cstr[i] == '*');
  if (!bullet) return false;

  i++;
  bool space_before_open_bracket = (i < length && cstr[i] == ' ');
  if (!space_before_open_bracket) return false;

  i++;
  bool open_bracket = (i < length && cstr[i] == '[');
  if (!open_bracket) return false;

  i++;
  bool status = (i < length &&
                    (cstr[i] == ' '     // Incomplete
                    || cstr[i] == 'x'   // Finished
                    || cstr[i] == '?'   // Question
                    || cstr[i] == '-'   // Working on it
                    || cstr[i] == '~')  // Removed / won't do
                );
  if (!status) return false;

  i++;
  bool close_bracket = (i < length && cstr[i] == ']');
  if (!close_bracket) return false;

  i++;
  bool space_before_title = (i < length && cstr[i] == ' ');
  if (!space_before_title) return false;

  return true;
}

bool is_property(const char *property_name, const char *cstr) {
  String_builder property_key = sb_create("%s: ", property_name);
  bool match = cstr_starts_with(cstr, property_key.str);
  sb_free(&property_key);
  return match;
}

char *read_property(const char *property_name, const char *cstr, unsigned int cstr_length, unsigned int *cursor) {
  String_builder property_key = sb_create("%s: ", property_name);
  *cursor += property_key.length;
  sb_free(&property_key);

  unsigned int end_line = *cursor;
  while (end_line < cstr_length && cstr[end_line] != '\n') end_line++;

  unsigned int value_length = end_line - *cursor;
  char *value = malloc(value_length + 1);
  if (!value) return NULL;
  strncpy(value, cstr + *cursor, value_length);
  value[value_length] = '\0';
  *cursor = end_line;

  return value;
}

bool parse_reminder(char *rem_str, Reminder *rem) {
  if (!rem_str || !rem) return false;

  Input rem_input = {
    .input = rem_str,
    .length = strlen(rem_str),
    .cursor = 0,
  };

  char *rem_date = next_token(&rem_input, ' ');
  if (!rem_date || rem_date[0] == '\0') {
    APPEND_TO_BACKTRACE(BACKTRACE_ERROR, "Unable to get the date of the reminder");
    if (rem_date) free(rem_date);
    return false;
  }

  if (!load_date_from_string(rem_date, &rem->start)) {
    APPEND_TO_BACKTRACE(BACKTRACE_ERROR, "Unable to parse the start date of the reminder");
    if (rem_date) free(rem_date);
    return false;
  }
  free(rem_date); rem_date = NULL;

  unsigned int marker = rem_input.cursor;
  char *divider = next_token(&rem_input, ' ');
  // Try to see if there is an end date
  if (divider && !strcmp(divider, "~")) {
    free(divider);

    rem_date = next_token(&rem_input, ' ');
    if (!rem_date || rem_date[0] == '\0') {
      APPEND_TO_BACKTRACE(BACKTRACE_ERROR, "Unable to get the end date of the reminder");
      if (rem_date) free(rem_date);
      return false;
    }

    if (!load_date_from_string(rem_date, &rem->end)) {
      APPEND_TO_BACKTRACE(BACKTRACE_ERROR, "Unable to parse the end date of the reminder");
      free(rem_date);
      return false;
    }
    free(rem_date);

  } else {
    if (divider) free(divider);
    rem_input.cursor = marker;
    rem->end = rem->start;
  }

  rem->name = next_token(&rem_input, '\0');
  if (!rem->name || rem->name[0] == '\0') {
    APPEND_TO_BACKTRACE(BACKTRACE_ERROR, "Unable to get the name of the reminder");
    if (rem->name) free(rem->name);
    return false;
  }

  return true;
}

bool get_attributes(Todo *todo, Attribute_type attr_type, List *attributes) {
  if (!todo) return false;
  if (!todo->notes) return true;

  unsigned int notes_length = strlen(todo->notes);
  unsigned int indentation = 0;

  bool new_line = true;
  unsigned int spaces = 0;
  for (unsigned int i=0; i < notes_length; i++) {
    const char c = todo->notes[i];

    if (c == '\n') {
      new_line = true;
      spaces = 0;
      continue;
    }

    if (!new_line) continue;

    const char *cstr_start = todo->notes + i;
    const unsigned int cstr_length = notes_length - i;

    if (c == ' ') {
      spaces++;
      continue;

    }

    switch (attr_type) {
      case ATTRIBUTE_TASK:
        if (is_a_task(cstr_start, cstr_length)){
          // Auto-detect indentation with the indentation of the first checkbox
          // that has some indentation
          if (indentation == 0 && spaces != 0) {
            indentation = spaces;
          }

          Task *task = malloc(sizeof(Task));
          task->todo = todo;
          task->level = (indentation) ? spaces / indentation : 0;
          task->state = *(todo->notes + i + 3);

          // Read the message from the checkbox
          i += 6; // strlen("- [x] ")
          unsigned int x = i;
          while (x < notes_length && todo->notes[x] != '\n') x++;
          unsigned int msg_length = x - i;
          if (msg_length == 0) {
            free(task);
            new_line = true;
            continue;
          }

          task->msg = malloc(msg_length + 1);
          if (!task->msg) return false;
          strncpy(task->msg, todo->notes + i, msg_length);
          task->msg[msg_length] = '\0';
          list_append(attributes, task);

          i += msg_length;
          new_line = true;
          spaces = 0;
          continue;
        }
        break;

      case ATTRIBUTE_TAG:
        if (is_property("tags", cstr_start)) {
          char *tags = read_property("tags", todo->notes, notes_length, &i);

          Input tags_input = {
            .input = tags,
            .cursor = 0,
            .length = strlen(tags),
          };
          char *tag = NULL;
          while ( (tag = next_token(&tags_input, ' ')) ) {
            // TODO Needed?
            // const unsigned int tag_length = strlen(tag);
            // if (tag[tag_length-1] == '\n') tag[tag_length-1] = '\0';
            list_append(attributes, tag);
          }

          free(tags);
          new_line = false;
          continue;
        }
        break;

      case ATTRIBUTE_REMINDER:
        if (is_property("reminder", cstr_start)) {
          char *reminder_cstr = read_property("reminder", todo->notes, notes_length, &i);

          Reminder *rem = malloc(sizeof(Reminder));
          memset(rem, 0, sizeof(Reminder));
          rem->todo = todo;
          bool ok = parse_reminder(reminder_cstr, rem);
          free(reminder_cstr); reminder_cstr = NULL;
          if (!ok) {
            APPEND_TO_BACKTRACE(BACKTRACE_ERROR, "Unable to parse the %dº reminder from the ToDo '%s'", attributes->count+1, todo->name);
            free(rem);
            return false;
          }

          list_insert_sorted(attributes, rem, (void *(*)(void *, void *)) reminder_insertion_comparator);
          new_line = false;
          continue;
        }
        break;
    }

    new_line = false;
  }

  return true;
}

bool is_reminder_old(Reminder rem) {
  const Date now = date_now();

  if (now.year < rem.end.year) return false;
  if (now.year > rem.end.year) return true;

  if (now.month < rem.end.month) return false;
  if (now.month > rem.end.month) return true;

  if (now.day < rem.end.day) return false;
  if (now.day > rem.end.day) return true;

  return false;
}

bool is_reminder_triggered(Reminder rem) {
  const Date now = date_now();
  return is_date_less_or_equals(rem.start, now) && is_date_less_or_equals(now, rem.end);
}

bool is_reminder_upcoming(Reminder rem) {
  const Date now = date_now();
  const int days_left = get_delta_time_days(now, rem.start);

  return 0 < days_left && days_left <= UPCOMING_REMINDER_DAYS;
}

void free_reminder(Reminder *rem) {
  free(rem->name);
  free(rem);
}

Reminder *reminder_insertion_comparator(Reminder *rem1, Reminder *rem2) {
  /// 1. Old reminder first
  if (is_reminder_old(*rem1) && !is_reminder_old(*rem2)) return rem1;
  if (is_reminder_old(*rem2) && !is_reminder_old(*rem1)) return rem2;

  /// 2. Triggered reminder first
  if (is_reminder_triggered(*rem1) && !is_reminder_triggered(*rem2)) return rem1;
  if (is_reminder_triggered(*rem2) && !is_reminder_triggered(*rem1)) return rem2;

  /// 3. If they are triggered, then the shorter reminder first
  if (is_reminder_triggered(*rem1)) {
    return (get_delta_time_days(rem1->start, rem1->end) >= get_delta_time_days(rem2->start, rem2->end))
           ? rem2 : rem1;
  }

  /// 4. Sort by start date (if it is in the future) or end date (if it is old)
  Date rem1_date = (is_reminder_old(*rem1)) ? rem1->end : rem1->start;
  Date rem2_date = (is_reminder_old(*rem2)) ? rem2->end : rem2->start;

  if (rem1_date.year < rem2_date.year) return rem1;
  if (rem2_date.year < rem1_date.year) return rem2;

  if (rem1_date.month < rem2_date.month) return rem1;
  if (rem2_date.month < rem1_date.month) return rem2;

  if (rem1_date.day < rem2_date.day) return rem1;
  if (rem2_date.day < rem1_date.day) return rem2;

  // It has more priority the ToDo that has less duration
  return (get_delta_time_days(rem1->start, rem1->end) >= get_delta_time_days(rem2->start, rem2->end))
         ? rem2 : rem1;
}

// bool get_all_reminders(List *reminders) {
//   if (!reminders || !list_is_empty(*reminders)) return false;
//
//   List_iterator todo_list_iterator = list_iterator_create(todo_list);
//   while (list_iterator_next(&todo_list_iterator)) {
//     Todo *todo = list_iterator_element(todo_list_iterator);
//
//     if (!get_reminders_from_todo(todo, reminders)) {
//       list_destroy(reminders, (void (*)(void *)) free_reminder);
//     }
//   }
//
//   return true;
// }

bool is_task_incomplete(Task task) {
  return task.state != 'x' && task.state != '~';
}

bool is_reminder_near(Reminder rem) {
  return is_reminder_old(rem) || is_reminder_triggered(rem) || is_reminder_upcoming(rem);
}

bool comparator_equals_tag(void *t1, void *t2) {
  return !strcmp(t1, t2);
}

bool get_attributes_from_todo_list(List todos, Attribute_type attr_type, List *attributes) {
  List_iterator todo_list_iterator = list_iterator_create(todos);
  while (list_iterator_next(&todo_list_iterator)) {
    Todo *todo = list_iterator_element(todo_list_iterator);

    List todo_attributes = list_new();
    if (!get_attributes(todo, attr_type, &todo_attributes)) {
      return false;
    }

    while (!list_is_empty(todo_attributes)) {
      void *attribute = list_remove(&todo_attributes, 0);

      switch (attr_type) {
        case ATTRIBUTE_TAG:
          if (!list_insert_if_unique(attributes, attribute, comparator_equals_tag)) free(attribute);
          break;

        case ATTRIBUTE_REMINDER:
          list_insert_sorted(attributes, attribute, (void *(*)(void *, void *)) reminder_insertion_comparator);
          break;

        case ATTRIBUTE_TASK:
          list_append(attributes, attribute);
          break;
      }
    }
  }

  return true;
}
