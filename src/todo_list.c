#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <time.h>

#include "main.h"
#include "parser.h"
#include "utils/list.h"
#include "utils/string.h"
#include "todo_list.h"
#include "template/template.h"

List todo_list = list_new();
bool todo_list_modified = false;
unsigned int instance_todo_counter = 0; // Counter of ToDos created by the current instance (to avoid id collisions in the same second)

bool search_todo_pos_by_name_or_pos(const char *name_or_position, unsigned int *index) { // `position` should be 1-based. `index` is 0-based
  if (list_is_empty(todo_list)) return false;

  *index = atoi(name_or_position);

  if (*index == 0) { // It's a string, not a number
    List_iterator iterator = list_iterator_create(todo_list);
    while (list_iterator_next(&iterator)) {
      const Todo *e = ((Todo *) list_iterator_element(iterator));
      if (!strcmp(e->name, name_or_position)) break;
    }

    if (list_iterator_finished(iterator)) return false;
    *index = list_iterator_index(iterator);
  } else {
    (*index)--; // 1-based index to 0-based index
    if (*index > list_size(todo_list)-1) return false;
  }

  return true;
}

char *generate_unique_todo_id() {
  char hostname[256];
  if (gethostname(hostname, sizeof(hostname)) == -1) return NULL;

  String_builder id_builder = sb_create("%s-%ld-%d", hostname, time(NULL), ++instance_todo_counter);
  return id_builder.str;
}

void free_todo(Todo *todo) {
  free(todo->name);
  free(todo->id);
  free(todo->notes);
  free(todo);
}

// id has to be NULL to be generated, otherwise is duplicated from the given id
Todo *create_todo(char *id) {
  Todo *new = malloc(sizeof(Todo));
  if (!new) return NULL;
  new->id = (id) ? strdup(id) : generate_unique_todo_id();
  new->name = NULL;
  new->notes = NULL;
  return new;
}

/// FILE OPERATIONS
bool save_string_to_binary_file(FILE *file, char *str) {
  if (!file) return false;
  unsigned int size = (str) ? strlen(str)+1 : 0;
  if (fwrite(&size, sizeof(unsigned int), 1, file) != 1) return false;
  if (size && fwrite(str, size, 1, file) != 1) return false;
  return true;
}

bool load_string_from_binary_file(FILE *file, char **str) {
  if (!file || !str) return false;
  unsigned int size;
  if (fread(&size, sizeof(unsigned int), 1, file) != 1) return false;
  if (size) {
    *str = malloc(size);
    if (fread(*str, size, 1, file) != 1) {
      free(str);
      return false;
    }
  } else {
    *str = NULL;
  }
  return true;
}

bool save_todo_to_binary_file(FILE *file, Todo *todo) {
  if (!save_string_to_binary_file(file, todo->id)) return false;
  if (!save_string_to_binary_file(file, todo->name)) return false;
  if (!save_string_to_binary_file(file, todo->notes)) return false;
  return true;
}

void *load_todo_from_binary_file(FILE *file) {
  Todo *todo = malloc(sizeof(Todo));
  if (!todo) abort();
  if (!load_string_from_binary_file(file, &todo->id)) return false;
  if (!load_string_from_binary_file(file, &todo->name)) return false;
  if (!load_string_from_binary_file(file, &todo->notes)) return false;
  return todo;
}

bool load_todo_from_export_file(FILE *load_file, List *old_todo_list, bool *reached_eof) {
  if (!load_file || !reached_eof || !old_todo_list) return false;
  bool ret = true;

  String_builder line = sb_new();
  char *atribute = NULL;
  Todo *new_todo = NULL;
  Todo *old_todo = NULL;
  String_builder todo_notes = sb_new();
  enum State {
    NO_STATE,
    STATE_PROPERTIES, // Reading properties of the ToDo
    STATE_NOTES_CONTENT, // Reading the content of the todo notes
    STATE_EXIT
  } state = NO_STATE;

  while ( state != STATE_EXIT && (sb_read_line(load_file, &line)) ) {
    if (!line.length) continue;

    unsigned int indentation = 0;
    while (cstr_starts_with(line.str + indentation * strlen(EXPORT_FILE_INDENTATION), EXPORT_FILE_INDENTATION)) indentation++;

    Input line_input = {
      .input = line.str,
      .cursor = indentation * strlen(EXPORT_FILE_INDENTATION),
      .length = line.length
    };
    atribute = next_token(&line_input, ' ');


    if (!indentation && !strcmp(atribute, "--")) {
      sb_clean(&line);
      free(atribute);
      continue;
    }

    switch (state) {
      case NO_STATE:
        if (!indentation && !strcmp(atribute, "todo")) {
          state = STATE_PROPERTIES;

        } else {
          ret = false;
          state = STATE_EXIT;
        }
        break;

      case STATE_PROPERTIES:
        if (!indentation && !strcmp(atribute, "todo")) {
          // Rewind what it read and break out of the function because it's
          // reading the next todo, and this function only reads one ToDo
          fseek(load_file, -1 * ((int) line.length + 1 /* \n */), SEEK_CUR);
          state = STATE_EXIT;
          break;

        } else if (indentation == 1 && !strcmp(atribute, "id:")) {
          if (new_todo) {
            ret = false;
            state = STATE_EXIT;
            break;
          }

          char *id = next_token(&line_input, 0);
          new_todo = create_todo(id);
          free(id);

          list_append(&todo_list, new_todo);

          // Search if the todo has a local version
          List_iterator iterator = list_iterator_create(*old_todo_list);
          while (list_iterator_next(&iterator)) {
            Todo *element = list_iterator_element(iterator);
            if (!strcmp(element->id, new_todo->id)) {
              old_todo = list_remove(old_todo_list, list_iterator_index(iterator));
              break;
            }
          }

        } else if (indentation == 1 && !strcmp(atribute, "name:")) {
          if (!new_todo || new_todo->name) {
            ret = false;
            state = STATE_EXIT;
            break;
          }

          new_todo->name = next_token(&line_input, 0);

        } else if (indentation == 1 && !strcmp(atribute, "notes_content:")) {
          if (!new_todo || new_todo->notes || !sb_is_empty(todo_notes)) {
            ret = false;
            state = STATE_EXIT;
            break;
          }

          state = STATE_NOTES_CONTENT;

        } else {
          ret = false;
          state = STATE_EXIT;
        }
        break;

      case STATE_NOTES_CONTENT:
        if (indentation == 2) {
          if (!new_todo) {
            ret = false;
            state = STATE_EXIT;
            break;
          }

          line_input.cursor = indentation * strlen(EXPORT_FILE_INDENTATION);
          char *file_content = next_token(&line_input, 0);
          if (!file_content) {
            sb_append(&todo_notes, "\n");
            break;
          }
          sb_append_with_format(&todo_notes, "%s\n", file_content);
          free(file_content);

        } else if (indentation == 1 && !strcmp(atribute, "EOF")) {
          new_todo->notes = todo_notes.str;
          todo_notes = sb_new();
          state = STATE_PROPERTIES;

        } else {
          sb_free(&todo_notes);
          ret = false;
          state = STATE_EXIT;
        }
        break;

      case STATE_EXIT:
        abort();
        break;
    }

    free(atribute); atribute = NULL;
    sb_clean(&line);
  }

  if (state == NO_STATE) {
    *reached_eof = true;
    ret = false;
  }

  if (!sb_is_empty(todo_notes)) {
    sb_free(&todo_notes);
    ret = false;
  }

  if (old_todo) {
    free_todo(old_todo);
  }

  sb_free(&line);
  return ret;
}

bool write_notes_to_export_file(FILE *save_file, Todo *todo) {
  if (!todo || !save_file) return false;
  if (!todo->notes) return true;

  if (fputs(EXPORT_FILE_INDENTATION "notes_content:\n" EXPORT_FILE_INDENTATION EXPORT_FILE_INDENTATION, save_file) == EOF) return false;

  unsigned int length = strlen(todo->notes);
  for (unsigned int i=0; i < length-1; i++) {
    char c = todo->notes[i];

    switch (c) {
      case '\n' :
        if (fputs("\n" EXPORT_FILE_INDENTATION EXPORT_FILE_INDENTATION, save_file) == EOF) return false;
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
  fputs("\n" EXPORT_FILE_INDENTATION "EOF\n", save_file);

  return true;
}

char *escape_backslash_cstr(String_builder *sb, char *cstr) {
  sb_clean(sb);

  unsigned int str_length = strlen(cstr);
  for (unsigned int i=0; i<str_length; i++) {
    if (cstr[i] == '\\') sb_append(sb, "\\\\");
    else sb_append_char(sb, cstr[i]);
  }

  return sb->str;
}

bool save_todo_to_export_file(FILE *file, Todo *todo) {
  if (fprintf(file, "todo\n") <= 0) return false;

  String_builder sb = sb_new();

  sb_append(&sb, todo->id);
  sb_search_and_replace(&sb, "\\", "\\\\");
  if (fprintf(file, EXPORT_FILE_INDENTATION "id: %s\n", sb.str) <= 0) return false;
  sb_clean(&sb);

  sb_append(&sb, todo->name);
  sb_search_and_replace(&sb, "\\", "\\\\");
  if (fprintf(file, EXPORT_FILE_INDENTATION "name: %s\n", sb.str) <= 0) return false;
  sb_clean(&sb);

  sb_free(&sb);

  if (todo->notes && !write_notes_to_export_file(file, todo)) return false;

  return true;
}

bool save_todo_list() {
  String_builder path = sb_create("%s/" SAVE_FILENAME, idea_state.local_path);
  FILE *save_file = fopen(path.str, "wb");
  sb_free(&path);
  if (!save_file) return false;

  if (!list_save_to_bfile(todo_list, (bool (*)(FILE *, void *)) save_todo_to_binary_file, save_file)) {
    fclose(save_file);
    return false;
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
      printf("[ERROR] Unable to create the directory %s\n", path);
      return false;
    }
  }

  return true;
}

bool load_todo_list() {
  if (!list_is_empty(todo_list)) abort();

  String_builder path = sb_create("%s/" SAVE_FILENAME, idea_state.local_path);

  FILE *save_file = fopen(path.str, "rb");
  sb_free(&path);
  if (!save_file) return true; // First time running it

  if (!list_load_from_bfile(&todo_list, load_todo_from_binary_file, save_file)) abort();

  fclose(save_file);
  return true;
}

/// Functionality
Action_return action_add_todo(Input *input) {
  char *data = next_token(input, 0);
  if (!data) return ACTION_RETURN(RETURN_ERROR, "Command malformed");

  Todo *todo = create_todo(NULL);
  if (!todo) return ACTION_RETURN(RETURN_ERROR_AND_EXIT, "No more memory");
  todo->name = data;

  list_append(&todo_list, todo);
  todo_list_modified = true;
  return ACTION_RETURN(RETURN_SUCCESS, "");
}

Action_return action_add_at_todo(Input *input) {
  char *pos_str = next_token(input, ' ');
  if (!pos_str) return ACTION_RETURN(RETURN_ERROR, "Command malformed");
  unsigned int pos = atoi(pos_str);
  free(pos_str);
  if (pos == 0 || pos > list_size(todo_list)+1) return ACTION_RETURN(RETURN_ERROR, "Invalid position");

  char *data = next_token(input, 0);
  if (!data) return ACTION_RETURN(RETURN_ERROR, "Command malformed");

  Todo *todo = create_todo(NULL);
  if (!todo) {
    free(data);
    return ACTION_RETURN(RETURN_ERROR_AND_EXIT, "No more memory");
  }
  todo->name = data;

  list_insert_at(&todo_list, todo, pos-1);
  todo_list_modified = true;
  return ACTION_RETURN(RETURN_SUCCESS, "");
}

Action_return action_remove_todo(Input *input) {
  char *argument = next_token(input, 0);
  if (!argument) return ACTION_RETURN(RETURN_ERROR, "Command malformed");

  unsigned int index;
  if (!search_todo_pos_by_name_or_pos(argument, &index)) {
    free(argument);
    return ACTION_RETURN(RETURN_ERROR, "Unable to find the ToDo");
  }
  free(argument); argument = NULL;

  Todo *removed = list_remove(&todo_list, index);
  todo_list_modified = true;

  free_todo(removed);
  return ACTION_RETURN(RETURN_SUCCESS, "");
}

Action_return action_move_todo(Input *input) {
  char *arg = next_token(input, ' ');
  if (!arg) return ACTION_RETURN(RETURN_ERROR, "You need to specify the Name or the ID of the ToDo to move");
  unsigned int pos_origin;
  if (!search_todo_pos_by_name_or_pos(arg, &pos_origin)) {
    free(arg);
    return ACTION_RETURN(RETURN_ERROR, "Unable to find the ToDo");
  }
  free(arg); arg = NULL;

  arg = next_token(input, 0);
  if (!arg) return ACTION_RETURN(RETURN_ERROR, "You need to specify the position to move the ToDo");
  unsigned int pos_destination = atoi(arg);
  free(arg); arg = NULL;
  if (pos_destination == 0 || pos_destination > list_size(todo_list)) return ACTION_RETURN(RETURN_ERROR, "Invalid destination position");

  if (pos_destination-1 /* 1-based to 0-based */ == pos_origin) return ACTION_RETURN(RETURN_INFO, "Moving to the same position");

  Todo *todo = list_remove(&todo_list, pos_origin);
  // if (pos_origin < pos_destination) pos_destination--;
  list_insert_at(&todo_list, todo, pos_destination-1);
  todo_list_modified = true;
  return ACTION_RETURN(RETURN_SUCCESS, "");
}

Action_return action_edit_todo(Input *input) {
  char *arg = next_token(input, ' ');
  if (!arg) return ACTION_RETURN(RETURN_ERROR, "You need to specify the Name or the ID of the ToDo to edit");

  unsigned int pos;
  if (!search_todo_pos_by_name_or_pos(arg, &pos)) {
    free(arg);
    return ACTION_RETURN(RETURN_ERROR, "Unable to find the ToDo");
  }
  free(arg); arg = NULL;

  char *new_text = next_token(input, 0);
  if (!new_text) return ACTION_RETURN(RETURN_ERROR, "The ToDo can't have an empty name.");

  Todo *todo = list_get(todo_list, pos);
  free(todo->name);
  todo->name = new_text;
  todo_list_modified = true;
  return ACTION_RETURN(RETURN_SUCCESS, "");
}

Action_return action_clear_todos(Input *input) {
  char *confirmation = next_token(input, 0);
  if (!confirmation) return ACTION_RETURN(RETURN_ERROR, "Command malformed");
  if (strcmp(confirmation, "all")) {
    free(confirmation);
    return ACTION_RETURN(RETURN_ERROR, "Specify 'all' as the first argument to clear all the ToDos");
  }
  free(confirmation);

  list_destroy(&todo_list, (void (*)(void *))free_todo);
  todo_list_modified = true;
  return ACTION_RETURN(RETURN_SUCCESS, "");
}

Action_return action_notes_todo_remove(Input *input) {
  char *arg = next_token(input, 0);
  if (!arg) return ACTION_RETURN(RETURN_ERROR, "Command malformed");

  unsigned int pos;
  if (!search_todo_pos_by_name_or_pos(arg, &pos)) {
    free(arg);
    return ACTION_RETURN(RETURN_ERROR, "Unable to find the ToDo");
  }
  Todo *todo = list_get(todo_list, pos);
  free(arg); arg = NULL;

  if (!todo->notes) return ACTION_RETURN(RETURN_ERROR, "The todo doesn't have a notes file");

  free(todo->notes);
  todo->notes = NULL;
  todo_list_modified = true;

  return ACTION_RETURN(RETURN_SUCCESS, "");
}

void initialize_notes(Todo *todo) {
  if (todo->notes) return;

  // Default ToDo note template
  String_builder sb = sb_create("# %s\n\n\n", todo->name);
  todo->notes = sb.str;
  todo_list_modified = true;
}

Action_return action_generate_html(Input *input) {
  List custom_todos = list_new();
  FILE *output_file = NULL;
  Action_return ret = ACTION_RETURN(RETURN_SUCCESS, "");

  char *output_path = next_token(input, ' ');
  if (!output_path) {
    ret = ACTION_RETURN(RETURN_ERROR, "You need to specify at least the HTML output path");
    goto exit;
  }

  char *arg = NULL;
  while ( (arg = next_token(input, ' ')) ) {
    unsigned int pos = 0;
    if (!search_todo_pos_by_name_or_pos(arg, &pos)) {
      ret = ACTION_RETURN(RETURN_ERROR, "Unable to find the ToDo");
      goto exit;
    }
    list_append(&custom_todos, list_get(todo_list, pos));
  }

  output_file = fopen(output_path, "w");
  if (!output_file) return ACTION_RETURN(RETURN_ERROR, "Unable to open the path of the output HTML file");

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
  { "remove", "rm", action_remove_todo, MAN("Remove a ToDo", "[ID]", "[name]") },
  { "move", "mv", action_move_todo, MAN("Move ToDo", "[ID] [new position]", "[name] [new position]") },
  { "edit", "e", action_edit_todo, MAN("Edit a ToDo", "[ID] [new name]", "[name] [new name]") },
  { "clear", NULL, action_clear_todos, MAN("Clear all ToDos", "all") },
  { "notes_remove", NULL, action_notes_todo_remove, MAN("Remove the notes file for the todo", "[ID]", "[name]") },
  { "html", NULL, action_generate_html, MAN("Generate an HTML file with the ToDos", "[HTML file path]") },
};
unsigned int todo_list_functionality_count = sizeof(todo_list_functionality) / sizeof(Functionality);
