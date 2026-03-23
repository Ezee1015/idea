#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>

#include "main.h"
#include "parser.h"
#include "utils/list.h"
#include "utils/string.h"
#include "todo_list.h"
#include "template/template.h"

bool is_a_number(const char *s) {
  for (int i = 0; s[i]; i++) if (s[i] < '0' || s[i] > '9') return false;
  return true;
}

bool search_todo_pos_by_name_or_pos(const char *name_or_position, unsigned int *index) { // `position` should be 1-based. `index` is 0-based
  if (list_is_empty(todo_list)) return false;

  if (is_a_number(name_or_position)) { // It's a string, not a number
    *index = atoi(name_or_position);
    (*index)--; // 1-based index to 0-based index
    if (*index > list_size(todo_list)-1) return false;
  } else {
    List_iterator iterator = list_iterator_create(todo_list);
    while (list_iterator_next(&iterator)) {
      const Todo *e = ((Todo *) list_iterator_element(iterator));
      if (!strcmp(e->name, name_or_position)) break;
    }

    if (list_iterator_finished(iterator)) return false;
    *index = list_iterator_index(iterator);
  }

  return true;
}

bool is_a_valid_todo_name(char *name) {
  if (!name) return false;

  if (todo_exists(name)) {
    APPEND_TO_BACKTRACE(BACKTRACE_ERROR, "Already exists a ToDo with the name '%s'", name);
    return false;
  }

  if (is_a_number(name)) {
    APPEND_TO_BACKTRACE(BACKTRACE_ERROR, "The name of the ToDo can't be just numbers");
    return false;
  }

  if (!strcmp(name, "")) {
    APPEND_TO_BACKTRACE(BACKTRACE_ERROR, "The name of the ToDo can't be empty");
    return false;
  }

  return true;
}

void free_todo(Todo *todo) {
  free(todo->name);
  free(todo->hostname);
  free(todo->notes);
  free(todo);
}

Todo *create_todo(char *name) {
  if (!name) return NULL;

  Todo *todo = malloc(sizeof(Todo));
  if (!todo) return NULL;

  todo->name = name;
  todo->creation_time = time(NULL);
  todo->notes = NULL;
  todo->hostname = strdup(idea_state.config.hostname);
  if (!todo->hostname) return NULL;

  return todo;
}

bool todo_exists(const char *name) {
  List_iterator iterator = list_iterator_create(todo_list);
  while (list_iterator_next(&iterator)) {
    Todo *element = list_iterator_element(iterator);
    if (!strcmp(element->name, name)) return true;
  }
  return false;
}

/// FILE OPERATIONS
bool load_todo_from_file(const char *load_file_path, FILE *load_file, bool *reached_eof) {
  if (!load_file_path || !load_file || !reached_eof) return false;
  bool ret = true;

  String_builder line = sb_new();
  unsigned int line_nr = 0;
  char *attribute = NULL;
  Todo *new_todo = NULL;
  String_builder todo_notes = sb_new();
  enum State {
    NO_STATE,
    STATE_PROPERTIES, // Reading properties of the ToDo
    STATE_NOTES_CONTENT, // Reading the content of the todo notes
    STATE_EXIT
  } state = NO_STATE;

  while ( state != STATE_EXIT && (sb_read_line(load_file, &line)) ) {
    line_nr++;
    if (!line.length) continue;

    unsigned int indentation = 0;
    while (cstr_starts_with(line.str + indentation * strlen(SAVE_FILE_INDENTATION), SAVE_FILE_INDENTATION)) indentation++;

    Input line_input = {
      .input = line.str,
      .cursor = indentation * strlen(SAVE_FILE_INDENTATION),
      .length = line.length
    };
    attribute = next_token(&line_input, ' ');
    if (!attribute) {
      // I can't just do 'continue' because if some ToDo has a
      // space at the beginning of some line in its notes, it
      // would not read that line.
      attribute = malloc(1);
      attribute[0] = '\0';
    }

    if (!indentation && !strcmp(attribute, "--")) {
      sb_clean(&line);
      if (attribute) free(attribute);
      continue;
    }

    switch (state) {
      case NO_STATE:
        if (!indentation && !strcmp(attribute, "todo")) {
          state = STATE_PROPERTIES;

        } else {
          ret = false;
          APPEND_TO_BACKTRACE(BACKTRACE_ERROR, "Import file %s:%u: You must start the file with a ToDo ('todo' key)", load_file_path, line_nr);
          state = STATE_EXIT;
        }
        break;

      case STATE_PROPERTIES:
        if (!indentation && !strcmp(attribute, "todo")) {
          // Rewind what it read and break out of the function because it's
          // reading the next todo, and this function only reads one ToDo
          fseek(load_file, -1 * ((int) line.length + 1 /* \n */), SEEK_CUR);
          state = STATE_EXIT;
          break;

        } else if (indentation == 1 && !strcmp(attribute, "name:")) {
          if (new_todo) {
            ret = false;
            APPEND_TO_BACKTRACE(BACKTRACE_ERROR, "Import file %s:%u: The name was already specified (%s)", load_file_path, line_nr, new_todo->name);
            state = STATE_EXIT;
            break;
          }

          char *name = next_token(&line_input, 0);


          if (!is_a_valid_todo_name(name)) {
            free(name);
            ret = false;
            APPEND_TO_BACKTRACE(BACKTRACE_ERROR, "Import file %s:%u: Invalid ToDo name", load_file_path, line_nr);
            state = STATE_EXIT;
            break;
          }

          new_todo = malloc(sizeof(Todo));
          memset(new_todo, 0, sizeof(Todo));
          new_todo->name = name;
          list_append(&todo_list, new_todo);

        } else if (indentation == 1 && !strcmp(attribute, "created:")) {
          if (!new_todo) {
            ret = false;
            APPEND_TO_BACKTRACE(BACKTRACE_ERROR, "Import file %s:%u: No ToDo specified", load_file_path, line_nr);
            state = STATE_EXIT;
            break;
          }

          if (new_todo->creation_time != 0) {
            ret = false;
            APPEND_TO_BACKTRACE(BACKTRACE_ERROR, "Import file %s:%u: Created time was already provided", load_file_path, line_nr);
            state = STATE_EXIT;
            break;
          }

          char *creation_time_cstr = next_token(&line_input, 0);
          if (!creation_time_cstr) {
            ret = false;
            APPEND_TO_BACKTRACE(BACKTRACE_ERROR, "Import file %s:%u: Creation time was not provided", load_file_path, line_nr);
            state = STATE_EXIT;
            break;
          }

          char *end = NULL;
          new_todo->creation_time = strtoull(creation_time_cstr, &end, 10);
          if (*end != '\0' || end == creation_time_cstr) {
            free(creation_time_cstr);
            ret = false;
            APPEND_TO_BACKTRACE(BACKTRACE_ERROR, "Import file %s:%u: Unable to parse the creation time", load_file_path, line_nr);
            state = STATE_EXIT;
            break;
          }
          free(creation_time_cstr);

        } else if (indentation == 1 && !strcmp(attribute, "hostname:")) {
          if (!new_todo) {
            ret = false;
            APPEND_TO_BACKTRACE(BACKTRACE_ERROR, "Import file %s:%u: No ToDo specified", load_file_path, line_nr);
            state = STATE_EXIT;
            break;
          }

          if (new_todo->hostname) {
            ret = false;
            APPEND_TO_BACKTRACE(BACKTRACE_ERROR, "Import file %s:%u: Host name already provided", load_file_path, line_nr);
            state = STATE_EXIT;
            break;
          }

          new_todo->hostname = next_token(&line_input, 0);

          if (!new_todo->hostname) {
            ret = false;
            APPEND_TO_BACKTRACE(BACKTRACE_ERROR, "Import file %s:%u: Empty hostname not allowed", load_file_path, line_nr);
            state = STATE_EXIT;
            break;
          }

        } else if (indentation == 1 && !strcmp(attribute, "notes_content:")) {
          if (!new_todo) {
            ret = false;
            APPEND_TO_BACKTRACE(BACKTRACE_ERROR, "Import file %s:%u: No ToDo specified", load_file_path, line_nr);
            state = STATE_EXIT;
            break;
          }

          if (new_todo->notes) {
            ret = false;
            APPEND_TO_BACKTRACE(BACKTRACE_ERROR, "Import file %s:%u: The ToDo already has notes", load_file_path, line_nr);
            state = STATE_EXIT;
            break;
          }

          if (!sb_is_empty(todo_notes)) abort();
          state = STATE_NOTES_CONTENT;

        } else {
          ret = false;
          APPEND_TO_BACKTRACE(BACKTRACE_ERROR, "Import file %s:%u: Unknown attribute '%s'", load_file_path, line_nr, attribute);
          state = STATE_EXIT;
        }
        break;

      case STATE_NOTES_CONTENT:
        if (indentation == 2) {
          if (!new_todo) {
            APPEND_TO_BACKTRACE(BACKTRACE_ERROR, "Import file %s:%u: No ToDo specified", load_file_path, line_nr);
            ret = false;
            state = STATE_EXIT;
            break;
          }

          line_input.cursor = indentation * strlen(SAVE_FILE_INDENTATION);
          char *file_content = next_token(&line_input, 0);
          if (!file_content) {
            sb_append(&todo_notes, "\n");
            break;
          }
          sb_append_with_format(&todo_notes, "%s\n", file_content);
          free(file_content);

        } else if (indentation == 1 && !strcmp(attribute, "EOF")) {
          new_todo->notes = todo_notes.str;
          todo_notes = sb_new();
          state = STATE_PROPERTIES;

        } else {
          sb_free(&todo_notes);
          APPEND_TO_BACKTRACE(BACKTRACE_ERROR, "Import file %s:%u: Incorrect indentation. Expected notes content line", load_file_path, line_nr);
          ret = false;
          state = STATE_EXIT;
        }
        break;

      case STATE_EXIT:
        abort();
        break;
    }

    free(attribute); attribute = NULL;
    sb_clean(&line);
  }

  if (state == NO_STATE) {
    *reached_eof = true;
    ret = false;
  }

  if (!sb_is_empty(todo_notes)) {
    sb_free(&todo_notes);
    ret = false;
    APPEND_TO_BACKTRACE(BACKTRACE_ERROR, "Import file %s:%u: Unclosed 'notes_content' (you need to end 'notes_content' with 'EOF')", load_file_path, line_nr);
  }

  sb_free(&line);
  return ret;
}

bool write_notes_to_file(FILE *save_file, Todo *todo) {
  if (!todo || !save_file) return false;
  if (!todo->notes) return true;

  if (fputs(SAVE_FILE_INDENTATION "notes_content:\n" SAVE_FILE_INDENTATION SAVE_FILE_INDENTATION, save_file) == EOF) return false;

  unsigned int length = strlen(todo->notes);
  for (unsigned int i=0; i < length-1; i++) {
    char c = todo->notes[i];

    switch (c) {
      case '\n' :
        if (fputs("\n" SAVE_FILE_INDENTATION SAVE_FILE_INDENTATION, save_file) == EOF) return false;
        break;

      case '\\':
        if (fputs("\\\\", save_file) == EOF) return false;
        break;

      default:
        if (fputc(c, save_file) == EOF) return false;
        break;
    }
  }

  // Every line in the notes content will always start with "| | [here is the content]"
  // I know a file ended when I encounter an "| EOF"
  if (fputs("\n" SAVE_FILE_INDENTATION "EOF\n", save_file) == EOF) return false;

  return true;
}

bool save_todo_to_file(FILE *file, Todo *todo) {
  if (fprintf(file, "todo\n") <= 0) {
    APPEND_TO_BACKTRACE(BACKTRACE_ERROR, "Unable to write to the export file");
    return false;
  }

  String_builder sb = sb_new();

  sb_append(&sb, todo->name);
  sb_search_and_replace(&sb, "\\", "\\\\");
  if (fprintf(file, SAVE_FILE_INDENTATION "name: %s\n", sb.str) <= 0) {
    APPEND_TO_BACKTRACE(BACKTRACE_ERROR, "Unable to write to the export file");
    return false;
  }
  sb_clean(&sb);

  if (todo->hostname) {
    sb_append(&sb, todo->hostname);
    sb_search_and_replace(&sb, "\\", "\\\\");
    if (fprintf(file, SAVE_FILE_INDENTATION "hostname: %s\n", sb.str) <= 0) {
      APPEND_TO_BACKTRACE(BACKTRACE_ERROR, "Unable to write to the export file");
      return false;
    }
    sb_clean(&sb);
  }

  if (fprintf(file, SAVE_FILE_INDENTATION "created: %lu\n", todo->creation_time) <= 0) {
    APPEND_TO_BACKTRACE(BACKTRACE_ERROR, "Unable to write to the export file");
    return false;
  }
  sb_clean(&sb);


  sb_free(&sb);

  if (todo->notes && !write_notes_to_file(file, todo)) {
    APPEND_TO_BACKTRACE(BACKTRACE_ERROR, "Unable to write the notes to the export file");
    return false;
  }

  return true;
}

bool save_todo_list(List list, char *file_path) {
  FILE *save_file = fopen(file_path, "w");
  if (!save_file) {
    APPEND_TO_BACKTRACE(BACKTRACE_ERROR, "Unable to create the save file '%s'", file_path);
    return false;
  }

  if (fputs("-- File generated by idea. Edit this file with caution.\n", save_file) == EOF) {
    fclose(save_file);
    APPEND_TO_BACKTRACE(BACKTRACE_ERROR, "Unable to save the file header");
    return false;
  }

  List_iterator iterator = list_iterator_create(list);
  while (list_iterator_next(&iterator)) {
    if (fputs("\n", save_file) == EOF) {
      fclose(save_file);
      APPEND_TO_BACKTRACE(BACKTRACE_ERROR, "Unable to print a new line in the export file");
      return false;
    }

    if (!save_todo_to_file(save_file, list_iterator_element(iterator))) {
      fclose(save_file);
      APPEND_TO_BACKTRACE(BACKTRACE_ERROR, "Unable to save all the ToDos in the save file");
      return false;
    }
  }

  fclose(save_file);
  return true;
}

bool create_dir_if_not_exists(char *dir_path) {
  struct stat st = {0};
  if (stat(dir_path, &st) != -1) return (S_ISDIR(st.st_mode));
  return (mkdir(dir_path, 0755) == 0);
}

bool create_dir_structure() {
  char *dirs[] = {
    idea_state.local_path,
  };

  unsigned int dirs_count = sizeof(dirs) / sizeof(char *);
  for (unsigned int i=0; i<dirs_count; i++) {
    char *path = dirs[i];
    if (!create_dir_if_not_exists(path)) {
      APPEND_TO_BACKTRACE(BACKTRACE_ERROR, "Unable to create the directory '%s'", path);
      return false;
    }
  }

  return true;
}

bool load_todo_list(List *list, char *file_path, bool obligatory) {
  FILE *save_file = fopen(file_path, "r");
  if (!save_file) {
    if (!obligatory) return true;

    APPEND_TO_BACKTRACE(BACKTRACE_ERROR, "Unable to open the save file '%s'", file_path);
    return false;
  }

  List old_list = *list;
  *list = list_new();

  bool reached_eof = false;
  while (load_todo_from_file(file_path, save_file, &reached_eof));
  fclose(save_file);

  if (reached_eof) {
    if (!list_is_empty(old_list)) list_destroy(&old_list, (void (*)(void *))free_todo);
  } else {
    APPEND_TO_BACKTRACE(BACKTRACE_ERROR, "An error has occured while parsing the ToDos file");
    if (!list_is_empty(*list)) list_destroy(list, (void (*)(void *))free_todo);
    *list = old_list;
  }

  return reached_eof;
}

/// Functionality
bool action_add_todo(Input *input) {
  char *data = next_token(input, 0);
  if (!data) {
    APPEND_TO_BACKTRACE(BACKTRACE_ERROR, "Command malformed: You must specify the name of the ToDo");
    return false;
  }

  if (!is_a_valid_todo_name(data)) {
    free(data);
    APPEND_TO_BACKTRACE(BACKTRACE_ERROR, "Invalid ToDo name");
    return false;
  }

  Todo *todo = create_todo(data);
  if (!todo) {
    free(data);
    return false;
  }

  list_append(&todo_list, todo);
  todo_list_modified = true;
  return true;
}

bool action_add_at_todo(Input *input) {
  char *pos_str = next_token(input, ' ');
  if (!pos_str) {
    APPEND_TO_BACKTRACE(BACKTRACE_ERROR, "Command malformed: You need to specify the index");
    return false;
  }
  unsigned int pos = atoi(pos_str);
  free(pos_str);
  if (pos == 0 || pos > list_size(todo_list)+1) {
    APPEND_TO_BACKTRACE(BACKTRACE_ERROR, "Invalid position (index)");
    return false;
  }

  char *data = next_token(input, 0);
  if (!data) {
    APPEND_TO_BACKTRACE(BACKTRACE_ERROR, "Command malformed: You need to specify the name of ToDo");
    return false;
  }

  if (!is_a_valid_todo_name(data)) {
    free(data);
    APPEND_TO_BACKTRACE(BACKTRACE_ERROR, "Invalid ToDo name");
    return false;
  }

  Todo *todo = create_todo(data);
  if (!todo) {
    free(data);
    return false;
  }

  list_insert_at(&todo_list, todo, pos-1);
  todo_list_modified = true;
  return true;
}

bool action_remove_todo(Input *input) {
  char *argument = next_token(input, 0);
  if (!argument) {
    APPEND_TO_BACKTRACE(BACKTRACE_ERROR, "Command malformed: You need to specify the ToDo");
    return false;
  }

  unsigned int index;
  if (!search_todo_pos_by_name_or_pos(argument, &index)) {
    APPEND_TO_BACKTRACE(BACKTRACE_ERROR, "Unable to find the ToDo '%s'", argument);
    free(argument);
    return false;
  }
  free(argument); argument = NULL;

  Todo *removed = list_remove(&todo_list, index);
  todo_list_modified = true;

  free_todo(removed);
  return true;
}

bool action_move_todo(Input *input) {
  char *arg = next_token(input, ' ');
  if (!arg) {
    APPEND_TO_BACKTRACE(BACKTRACE_ERROR, "Command malformed: You need to specify the Name or the ID of the ToDo to move");
    return false;
  }
  unsigned int pos_origin;
  if (!search_todo_pos_by_name_or_pos(arg, &pos_origin)) {
    APPEND_TO_BACKTRACE(BACKTRACE_ERROR, "Unable to find the ToDo '%s'", arg);
    free(arg);
    return false;
  }
  free(arg); arg = NULL;

  arg = next_token(input, 0);
  if (!arg) {
    APPEND_TO_BACKTRACE(BACKTRACE_ERROR, "You need to specify the position to move the ToDo");
    return false;
  }
  unsigned int pos_destination = atoi(arg);
  free(arg); arg = NULL;
  if (pos_destination == 0 || pos_destination > list_size(todo_list)) {
    APPEND_TO_BACKTRACE(BACKTRACE_ERROR, "Invalid destination position");
    return false;
  }

  if (pos_destination-1 /* 1-based to 0-based */ == pos_origin) {
    APPEND_TO_BACKTRACE(BACKTRACE_INFO, "Moving to the same position");
    return true;
  }

  Todo *todo = list_remove(&todo_list, pos_origin);
  // if (pos_origin < pos_destination) pos_destination--;
  list_insert_at(&todo_list, todo, pos_destination-1);
  todo_list_modified = true;
  return true;
}

bool action_edit_todo(Input *input) {
  char *arg = next_token(input, ' ');
  if (!arg) {
    APPEND_TO_BACKTRACE(BACKTRACE_ERROR, "You need to specify the Name or the ID of the ToDo to edit");
    return false;
  }

  unsigned int pos;
  if (!search_todo_pos_by_name_or_pos(arg, &pos)) {
    APPEND_TO_BACKTRACE(BACKTRACE_ERROR, "Unable to find the ToDo '%s'", arg);
    free(arg);
    return false;
  }
  free(arg); arg = NULL;

  char *new_name = next_token(input, 0);
  if (!new_name) {
    free(new_name);
    APPEND_TO_BACKTRACE(BACKTRACE_ERROR, "The ToDo can't have an empty name.");
    return false;
  }

  if (!is_a_valid_todo_name(new_name)) {
    free(new_name);
    APPEND_TO_BACKTRACE(BACKTRACE_ERROR, "Invalid ToDo name");
    return false;
  }

  Todo *todo = list_get(todo_list, pos);
  free(todo->name);
  todo->name = new_name;
  todo_list_modified = true;
  return true;
}

bool action_clear_todos(Input *input) {
  char *confirmation = next_token(input, 0);
  if (!confirmation) {
    APPEND_TO_BACKTRACE(BACKTRACE_ERROR, "Command malformed: you must confirm the operation by passing as an argument 'all'");
    return false;
  }
  if (strcmp(confirmation, "all")) {
    free(confirmation);
    APPEND_TO_BACKTRACE(BACKTRACE_ERROR, "Specify 'all' as the first argument to clear all the ToDos");
    return false;
  }
  free(confirmation);

  list_destroy(&todo_list, (void (*)(void *))free_todo);
  todo_list_modified = true;
  return true;
}

bool action_notes_todo_remove(Input *input) {
  char *arg = next_token(input, 0);
  if (!arg) {
    APPEND_TO_BACKTRACE(BACKTRACE_ERROR, "Command malformed: You need to specify the ToDo");
    return false;
  }

  unsigned int pos;
  if (!search_todo_pos_by_name_or_pos(arg, &pos)) {
    APPEND_TO_BACKTRACE(BACKTRACE_ERROR, "Unable to find the ToDo '%s'", arg);
    free(arg);
    return false;
  }
  Todo *todo = list_get(todo_list, pos);
  free(arg); arg = NULL;

  if (!todo->notes) {
    APPEND_TO_BACKTRACE(BACKTRACE_ERROR, "The todo doesn't have a notes file");
    return false;
  }

  free(todo->notes);
  todo->notes = NULL;
  todo_list_modified = true;

  return true;
}

void initialize_notes(Todo *todo) {
  if (todo->notes) return;

  // Default ToDo note template
  String_builder sb = sb_create("# %s\n\ntags: \n\n---\n\n\n", todo->name);
  todo->notes = sb.str;
  todo_list_modified = true;
}

bool action_generate_html(Input *input) {
  List custom_todos = list_new();
  FILE *output_file = NULL;
  bool ret = true;

  char *output_path = next_token(input, ' ');
  if (!output_path) {
    APPEND_TO_BACKTRACE(BACKTRACE_ERROR, "You need to specify at least the HTML output path");
    ret = false;
    goto exit;
  }

  char *arg = NULL;
  while ( (arg = next_token(input, ' ')) ) {
    unsigned int pos = 0;
    bool ok = search_todo_pos_by_name_or_pos(arg, &pos);

    if (!ok) {
      APPEND_TO_BACKTRACE(BACKTRACE_ERROR, "Unable to find the ToDo '%s'", arg);
      ret = false;
      free(arg);
      goto exit;
    }
    free(arg);

    list_append(&custom_todos, list_get(todo_list, pos));
  }

  output_file = fopen(output_path, "w");
  if (!output_file) {
    APPEND_TO_BACKTRACE(BACKTRACE_ERROR, "Unable to open the path of the output HTML file (%s)", output_path);
    ret = false;
    goto exit;
  }

  ret = generate_html(output_file, (!list_is_empty(custom_todos)) ? custom_todos : todo_list);
  list_destroy(&custom_todos, NULL);

exit:
  list_destroy(&custom_todos, NULL);
  if (output_file) fclose(output_file);
  if (output_path) free(output_path);
  return ret;
}

Functionality todo_list_functionality[] = {
  { "add", "a", action_add_todo, MAN("Add a ToDo", "[name]") },
  { "add_at", "ap", action_add_at_todo, MAN("Add a ToDo in the specified position", "[index] [name]") },
  { "remove", "rm", action_remove_todo, MAN("Remove a ToDo", "[index]", "[name]") },
  { "move", "mv", action_move_todo, MAN("Move ToDo", "[ID] [new position]", "[name] [new position]") },
  { "edit", "e", action_edit_todo, MAN("Edit a ToDo", "[ID] [new name]", "[name] [new name]") },
  { "clear", NULL, action_clear_todos, MAN("Clear all ToDos", "all") },
  { "notes_remove", NULL, action_notes_todo_remove, MAN("Remove the notes file for the todo", "[ID]", "[name]") },
  { "html", NULL, action_generate_html, MAN("Generate an HTML file with the ToDos", "[HTML file path]", "[HTML file path] [ToDo name 1] [ToDo name 2] [...]", "[HTML file path] [ToDo index 1] [ToDo index 2] [...]") },
};
unsigned int todo_list_functionality_count = sizeof(todo_list_functionality) / sizeof(Functionality);

void free_task(Task *task) {
  free(task->msg);
  free(task);
}

bool is_a_task(char *cstr, unsigned int length) {
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

List get_tasks_from_todo(Todo *todo) {
  if (!todo || !todo->notes) return list_new();

  List tasks = list_new();
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

    if (c == ' ') {
      spaces++;

    } else if (is_a_task(todo->notes+i, notes_length - i)){
      // Auto-detect indentation with the indentation of the first checkbox
      // that has some indentation
      if (indentation == 0 && spaces != 0) {
        indentation = spaces;
      }

      Task *t = malloc(sizeof(Task));
      t->level = (indentation) ? spaces / indentation : 0;
      t->state = *(todo->notes + i + 3);
      t->todo = todo;

      // Read the message from the checkbox
      i += 6;
      unsigned int x = i;
      while (x < notes_length && todo->notes[x] != '\n') x++;
      unsigned int msg_length = x - i;
      if (msg_length == 0) {
        free(t);
        new_line = true;
        continue;
      }

      t->msg = malloc(msg_length + 1);
      strncpy(t->msg, todo->notes + i, msg_length);
      t->msg[msg_length] = '\0';
      list_append(&tasks, t);

      new_line = true;
      spaces = 0;

    } else {
      new_line = false;
    }
  }

  return tasks;
}

List get_attribute_from_todo(Todo todo, const char *attribute, char argument_separator) {
  if (!todo.notes) return list_new();

  List tags = list_new();
  unsigned int notes_length = strlen(todo.notes);

  bool new_line = true;
  for (unsigned int i=0; i < notes_length; i++) {
    const char c = todo.notes[i];

    if (c == '\n') {
      new_line = true;
      continue;
    }

    if (!new_line || c == ' ') continue;

    if (cstr_starts_with(todo.notes+i, attribute)) {
      i += strlen(attribute);
      unsigned int end_tags = i;
      while (end_tags < notes_length && todo.notes[end_tags] != '\n') end_tags++;

      Input tags_input = {
        .input = todo.notes + i,
        .cursor = 0,
        .length = end_tags - i,
      };
      char *tag = NULL;
      while ( (tag = next_token(&tags_input, argument_separator)) ) {
        const unsigned int tag_length = strlen(tag);
        if (tag[tag_length-1] == '\n') tag[tag_length-1] = '\0';
        list_append(&tags, tag);
      }
    }

    new_line = false;
  }

  return tags;
}

bool parse_reminder(Todo *todo, char *rem_str, Reminder *rem) {
  if (!rem_str || !rem) return false;

  rem->todo = todo;

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

bool get_reminders_from_todo(Todo *todo, List *reminders) {
  List reminders_todo = get_attribute_from_todo(*todo, "reminder: ", '\0');

  unsigned int i = 0;
  while (!list_is_empty(reminders_todo)) {
    char *rem_str = list_remove(&reminders_todo, 0);

    Reminder *rem = malloc(sizeof(Reminder));
    memset(rem, 0, sizeof(Reminder));

    bool ok = parse_reminder(todo, rem_str, rem);
    free(rem_str); rem_str = NULL;
    if (!ok) {
      APPEND_TO_BACKTRACE(BACKTRACE_ERROR, "Unable to parse the %dº reminder from the ToDo '%s'", i+1, todo->name);
      free(rem);
      list_destroy(&reminders_todo, free);
      return false;
    }

    list_insert_sorted(reminders, rem, (void *(*)(void *, void *)) reminder_insertion_comparator);
    i++;
  }

  list_destroy(&reminders_todo, free);
  return true;
}

bool get_all_reminders(List *reminders) {
  if (!reminders || !list_is_empty(*reminders)) return false;

  List_iterator todo_list_iterator = list_iterator_create(todo_list);
  while (list_iterator_next(&todo_list_iterator)) {
    Todo *todo = list_iterator_element(todo_list_iterator);

    if (!get_reminders_from_todo(todo, reminders)) {
      list_destroy(reminders, (void (*)(void *)) free_reminder);
    }
  }

  return true;
}

bool is_task_incomplete(Task task) {
  return task.state != 'x' && task.state != '~';
}

bool get_all_tasks(List *tasks) {
  if (!tasks || !list_is_empty(*tasks)) return false;

  List_iterator todo_list_iterator = list_iterator_create(todo_list);
  while (list_iterator_next(&todo_list_iterator)) {
    Todo *todo = list_iterator_element(todo_list_iterator);

    List todo_tasks = get_tasks_from_todo(todo);
    while (!list_is_empty(todo_tasks)) {
      Task *task = list_remove(&todo_tasks, 0);
      list_append(tasks, task);
    }
  }

  return true;
}

bool comparator_equals_tag(void *t1, void *t2) {
  return !strcmp(t1, t2);
}

List get_all_tags(List todos) {
  List tags = list_new();
  List_iterator todo_list_iterator = list_iterator_create(todos);
  while (list_iterator_next(&todo_list_iterator)) {
    Todo *todo = list_iterator_element(todo_list_iterator);

    List todo_tags = get_attribute_from_todo(*todo, "tags: ", ' ');
    while (!list_is_empty(todo_tags)) {
      char *tag = list_remove(&todo_tags, 0);

      if(!list_insert_if_unique(&tags, tag, comparator_equals_tag)) free(tag);
    }
  }

  return tags;
}
