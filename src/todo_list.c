#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <time.h>

#include "parser.h"
#include "utils/list.h"
#include "todo_list.h"

List todo_list = list_new();
bool todo_list_modified = false;
unsigned int instance_todo_counter = 0; // Counter of ToDos created by the current instance (to avoid id collisions in the same second)

unsigned int digit_count(long n) {
  if (n == 0) return 1;
  if (n < 0) n = n * (-1);

  unsigned int c = 0;
  while (n > 0) {
    n /= 10;
    c++;
  }

  return c;
}

char *generate_unique_todo_id() {
  char hostname[256];
  if (gethostname(hostname, sizeof(hostname)) == -1) return NULL;
  time_t t = time(NULL);

  unsigned int id_length = strlen(hostname) + 1 + digit_count(t) + 1 + digit_count(++instance_todo_counter);
  char *id = malloc(id_length + 1);
  if (!id) return NULL;
  sprintf(id, "%s-%ld-%d", hostname, t, instance_todo_counter);
  return id;
}

void free_todo(Todo *todo) {
  free(todo->name);
  free(todo->id);
  free(todo);
}

bool remove_todo_notes(Todo *todo) {
  if (!todo->notes) return true;

  char *notes_directory = get_path_from_variable("HOME", CONFIG_PATH NOTES_FOLDER);
  char *file_path_without_ext = concatenate_paths(notes_directory, todo->id);
  free(notes_directory);
  char *path = malloc(strlen(file_path_without_ext) + 4);
  sprintf(path, "%s.md", file_path_without_ext);
  free(file_path_without_ext); file_path_without_ext = NULL;

  int remove_ret = remove(path);
  free(path);
  if (remove_ret == -1) return false;

  todo->notes = false;
  return true;
}

// id has to be NULL to be generated, otherwise is duplicated from the given id
Todo *create_todo(char *id) {
  Todo *new = malloc(sizeof(Todo));
  if (!new) return NULL;
  new->id = (id) ? strdup(id) : generate_unique_todo_id();
  new->name = NULL;
  new->notes = false;
  return new;
}

/// FILE OPERATIONS
char *read_line(FILE *f) {
  if (!f) abort();
  long original_pos = ftell(f);
  if (original_pos == -1) abort();

  unsigned int line_count = 2; // '\n' and '\0' character are always at the end when fgets reads the line
  int c;
  while ( (c = fgetc(f)) != EOF && c != '\n') line_count++;

  fseek(f, original_pos, SEEK_SET);

  char *str = malloc(line_count);
  if (!fgets(str, line_count, f)) {
    free(str);
    return NULL;
  }

  str[line_count-2] = '\0'; // remove new line from fgets
  return str;
}

char *concatenate_paths(const char *directory, const char *relative) {
  unsigned int directory_length = strlen(directory);
  unsigned int relative_length = strlen(relative);

  char *full_path = NULL;
  if (directory[directory_length-1] == '/') {
    full_path = malloc(directory_length + relative_length + 1);
    if (!full_path) return NULL;
    strcpy(full_path, directory);
    strcat(full_path, relative);
  } else {
    full_path = malloc(directory_length + 1 + relative_length + 1);
    if (!full_path) return NULL;
    sprintf(full_path, "%s/%s", directory, relative);
  }
  return full_path;
}

char *get_path_from_variable(const char *variable, const char *path_from_directory) {
  const char *directory_path = getenv(variable);
  if (!directory_path) return NULL;

  return concatenate_paths(directory_path, path_from_directory);
}

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
  if (!fwrite(&todo->notes, sizeof(bool), 1, file)) return false;
  return true;
}

void *load_todo_from_binary_file(FILE *file) {
  Todo *todo = malloc(sizeof(Todo));
  if (!todo) abort();
  if (!load_string_from_binary_file(file, &todo->id)) return false;
  if (!load_string_from_binary_file(file, &todo->name)) return false;
  if (!fread(&todo->notes, sizeof(bool), 1, file)) return false;
  return todo;
}

bool load_todo_from_text_file(FILE *load_file, List *old_todo_list, bool *reached_eof) {
  if (!load_file || !reached_eof || !old_todo_list) return false;
  bool ret = true;

  char *line;
  char *atribute = NULL;
  Todo *new_todo = NULL;
  Todo *old_todo = NULL;
  FILE *todo_notes = NULL;
  enum State {
    NO_STATE,
    STATE_PROPERTIES, // Reading properties of the ToDo
    STATE_NOTES_CONTENT, // Reading the content of the todo notes
    STATE_EXIT
  } state = NO_STATE;

  while ( state != STATE_EXIT && (line = read_line(load_file)) ) {
    // Finished reading ToDo
    if (!strcmp(line, "")) {
      free(line);
      state = STATE_EXIT;
      break;
    }

    unsigned int indentation = 0;
    while (str_starts_with(line + indentation * strlen(EXPORT_FILE_INDENTATION), EXPORT_FILE_INDENTATION)) indentation++;

    Input line_input = {
      .input = line,
      .cursor = indentation * strlen(EXPORT_FILE_INDENTATION),
      .length = strlen(line)
    };
    atribute = next_token(&line_input, ' ');


    if (!indentation && !strcmp(atribute, "--")) {
      free(line);
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
          fseek(load_file, -1 * (strlen(line) + 1 /* \n */), SEEK_CUR);
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
          if (!new_todo || new_todo->notes || todo_notes) {
            ret = false;
            state = STATE_EXIT;
            break;
          }

          char *notes_directory = get_path_from_variable("HOME", CONFIG_PATH NOTES_FOLDER);
          char *file_path_without_ext = concatenate_paths(notes_directory, new_todo->id);
          free(notes_directory); notes_directory = NULL;
          char *path = malloc(strlen(file_path_without_ext) + 4);
          sprintf(path, "%s.md", file_path_without_ext);
          free(file_path_without_ext); file_path_without_ext = NULL;

          todo_notes = fopen(path, "w");
          free(path); path = NULL;
          if (!todo_notes) { ret = false; state = STATE_EXIT; break; }
          new_todo->notes = true;
          state = STATE_NOTES_CONTENT;

        } else {
          ret = false;
          state = STATE_EXIT;
        }
        break;

      case STATE_NOTES_CONTENT:
        if (indentation == 2) {
          if (!new_todo || !new_todo->notes || !todo_notes) {
            ret = false;
            state = STATE_EXIT;
            break;
          }

          line_input.cursor = indentation * strlen(EXPORT_FILE_INDENTATION);
          char *file_content = next_token(&line_input, 0);
          if (!file_content) {
            fputs("\n", todo_notes);
            break;
          }
          fprintf(todo_notes, "%s\n", file_content);
          free(file_content);

        } else if (indentation == 1 && !strcmp(atribute, "EOF")) {
          fclose(todo_notes); todo_notes = NULL;
          state = STATE_PROPERTIES;

        } else {
          fclose(todo_notes); todo_notes = NULL;
          ret = false;
          state = STATE_EXIT;
        }
        break;

      case STATE_EXIT:
        abort();
        break;
    }

    free(atribute); atribute = NULL;
    free(line); line = NULL;
  }

  if (state == NO_STATE) {
    *reached_eof = true;
    ret = false;
  }

  if (todo_notes) {
    fclose(todo_notes);
    ret = false;
  }

  if (old_todo) {
    if (old_todo->notes && !new_todo->notes) remove_todo_notes(old_todo);
    free_todo(old_todo);
  }
  return ret;
}

bool save_todo_notes_to_text_file(FILE *save_file, Todo *todo) {
  if (!todo || !save_file) return false;
  if (!todo->notes) return true;

  char *notes_directory = get_path_from_variable("HOME", CONFIG_PATH NOTES_FOLDER);
  char *file_path_without_ext = concatenate_paths(notes_directory, todo->id);
  free(notes_directory); notes_directory = NULL;
  char *path = malloc(strlen(file_path_without_ext) + 4);
  sprintf(path, "%s.md", file_path_without_ext);
  free(file_path_without_ext); file_path_without_ext = NULL;

  FILE *notes = fopen(path, "r");
  free(path); path = NULL;
  if (!notes) return false;

  if (fputs(EXPORT_FILE_INDENTATION "notes_content:\n" EXPORT_FILE_INDENTATION EXPORT_FILE_INDENTATION, save_file) == EOF) { fclose(notes); return false; }
  int prev = fgetc(notes), cur;

  while ( (cur = fgetc(notes)) != EOF ) {
    switch (prev) {
      case '\n' :
        if (fputs("\n" EXPORT_FILE_INDENTATION EXPORT_FILE_INDENTATION, save_file) == EOF) { fclose(notes); return false; }
        break;

      default:
        if (fputc(prev, save_file) == EOF) { fclose(notes); return false; }
        break;
    }

    prev = cur;
  }

  // Every line in the notes content will always start with "| | [here is the content]"
  // I know a file ended when I encounter an "| EOF"
  fputs("\n" EXPORT_FILE_INDENTATION "EOF\n", save_file);

  fclose(notes);
  return true;
}

bool write_notes_to_notes_file(FILE *file, char *notes) {
  unsigned int content_lenght = strlen(notes);
  bool escaped = false;
  for (unsigned int i=0; i<content_lenght; i++) {
    switch (notes[i]) {
      case '\\':
        if (escaped && fputs("\\\\", file) == EOF) return false;
        escaped = !escaped;
        break;

      case 'n':
        if (fputs( (escaped) ? "\n" : "n", file) == EOF) return false;
        escaped = false;
        break;

      default:
        if (escaped) {
          if (fputc('\\', file) == EOF) return false;
          escaped = false;
        }
        if (fputc(notes[i], file) == EOF) return false;
        break;
    }
  }
  return true;
}

bool save_todo_to_text_file(FILE *file, Todo *todo) {
  if (fprintf(file, "todo\n") <= 0) return false;
  if (fprintf(file, EXPORT_FILE_INDENTATION "id: %s\n", todo->id) <= 0) return false;
  if (fprintf(file, EXPORT_FILE_INDENTATION "name: %s\n", todo->name) <= 0) return false;

  if (todo->notes) {
    if (!save_todo_notes_to_text_file(file, todo)) return false;
  }

  return true;
}

bool save_file() {
  char *path = get_path_from_variable("HOME", CONFIG_PATH SAVE_FILENAME);
  FILE *save_file = fopen(path, "wb");
  free(path);
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
  List paths = list_new();

  list_append(&paths, get_path_from_variable("HOME", CONFIG_PATH));

  char *notes_path = get_path_from_variable("HOME", CONFIG_PATH NOTES_FOLDER);
  notes_path[strlen(notes_path) - 1] = '\0'; // Remove the "/" at the end
  list_append(&paths, notes_path);

  while (!list_is_empty(paths)) {
    char *path = list_remove(&paths, 0);

    if (!create_dir_if_not_exists(path)) {
      printf("Unable to create the directory %s\n", path);
      free(path);
      list_destroy(&paths, free);
      return false;
    }
    free(path);
  }
  return true;
}

bool load_file() {
  if (!list_is_empty(todo_list)) abort();

  if (!create_dir_structure()) return false;

  char *path = get_path_from_variable("HOME", CONFIG_PATH SAVE_FILENAME);
  FILE *save_file = fopen(path, "rb");
  free(path);
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
  return ACTION_RETURN(RETURN_SUCCESS, "");
}

Action_return action_add_at_todo(Input *input) {
  char *pos_str = next_token(input, ' ');
  if (!pos_str) return ACTION_RETURN(RETURN_ERROR, "Command malformed");
  unsigned int pos = atoi(pos_str);
  free(pos_str);
  if (pos == 0 || pos > list_size(todo_list)) return ACTION_RETURN(RETURN_ERROR, "Invalid position");

  char *data = next_token(input, 0);
  if (!data) return ACTION_RETURN(RETURN_ERROR, "Command malformed");

  Todo *todo = create_todo(NULL);
  if (!todo) {
    free(data);
    return ACTION_RETURN(RETURN_ERROR_AND_EXIT, "No more memory");
  }
  todo->name = data;

  list_insert_at(&todo_list, todo, pos-1);
  return ACTION_RETURN(RETURN_SUCCESS, "");
}

Action_return action_remove_todo(Input *input) {
  char *pos_str = next_token(input, 0);
  if (!pos_str) return ACTION_RETURN(RETURN_ERROR, "Command malformed");
  unsigned int pos = atoi(pos_str);
  free(pos_str);

  if (pos == 0 || pos > list_size(todo_list)) return ACTION_RETURN(RETURN_ERROR, "Invalid Position");

  Todo *todo = list_remove(&todo_list, pos-1);

  if (!remove_todo_notes(todo)) {
    list_insert_at(&todo_list, todo, pos-1);
    return ACTION_RETURN(RETURN_ERROR, "Unable to remove the ToDo's notes file (inside ~/" CONFIG_PATH NOTES_FOLDER ")");
  }

  free_todo(todo);
  return ACTION_RETURN(RETURN_SUCCESS, "");
}

Action_return action_move_todo(Input *input) {
  char *pos_origin_str = next_token(input, ' ');
  if (!pos_origin_str) return ACTION_RETURN(RETURN_ERROR, "Command malformed");
  unsigned int pos_origin = atoi(pos_origin_str);
  free(pos_origin_str);

  char *pos_destination_str = next_token(input, 0);
  if (!pos_destination_str) return ACTION_RETURN(RETURN_ERROR, "Command malformed");
  unsigned int pos_destination = atoi(pos_destination_str);
  free(pos_destination_str);

  if (pos_origin == 0 || pos_origin > list_size(todo_list)) return ACTION_RETURN(RETURN_ERROR, "Invalid origin position");
  if (pos_destination == 0 || pos_destination > list_size(todo_list)) return ACTION_RETURN(RETURN_ERROR, "Invalid destination position");
  if (pos_destination == pos_origin) return ACTION_RETURN(RETURN_INFO, "Moving to the same position");

  Todo *todo = list_remove(&todo_list, pos_origin-1);
  // if (pos_origin < pos_destination) pos_destination--;
  list_insert_at(&todo_list, todo, pos_destination-1);
  return ACTION_RETURN(RETURN_SUCCESS, "");
}

Action_return action_edit_todo(Input *input) {
  char *pos_str = next_token(input, ' ');
  if (!pos_str) return ACTION_RETURN(RETURN_ERROR, "Command malformed");
  unsigned int pos = atoi(pos_str);
  free(pos_str);

  if (pos == 0 || pos > list_size(todo_list)) return ACTION_RETURN(RETURN_ERROR, "Invalid Position");

  char *new_text = next_token(input, 0);
  if (!new_text) return ACTION_RETURN(RETURN_ERROR, "Empty new text.");

  Todo *todo = list_get(todo_list, pos-1);
  free(todo->name);
  todo->name = new_text;
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

  if (!list_map_bool(todo_list, (bool (*)(void *))remove_todo_notes)) return ACTION_RETURN(RETURN_ERROR, "Unable to remove a ToDo's notes file");
  list_destroy(&todo_list, (void (*)(void *))free_todo);
  return ACTION_RETURN(RETURN_SUCCESS, "");
}

Action_return action_notes_todo_remove(Input *input) {
  char *pos_str = next_token(input, 0);
  if (!pos_str) return ACTION_RETURN(RETURN_ERROR, "Command malformed");
  unsigned int pos = atoi(pos_str);
  free(pos_str);
  if (!pos) return ACTION_RETURN(RETURN_ERROR, "The given position is not a number");
  if (pos == 0 || pos > list_size(todo_list)) return ACTION_RETURN(RETURN_ERROR, "Invalid position");
  Todo *todo = list_get(todo_list, pos-1);

  if (!todo->notes) return ACTION_RETURN(RETURN_INFO, "The todo doesn't have a notes file");
  if (!remove_todo_notes(todo)) return ACTION_RETURN(RETURN_ERROR, "Unable to remove the notes");
  return ACTION_RETURN(RETURN_SUCCESS, "");
}

// The opening of the notes is delegated to each interface
bool create_notes_todo(Todo *todo) {
  if (!todo || todo->notes) return false;

  char *notes_directory = get_path_from_variable("HOME", CONFIG_PATH NOTES_FOLDER);
  char *file_path_without_ext = concatenate_paths(notes_directory, todo->id);
  free(notes_directory); notes_directory = NULL;
  char *path = malloc(strlen(file_path_without_ext) + 4);
  sprintf(path, "%s.md", file_path_without_ext);
  free(file_path_without_ext);

  FILE *notes = fopen(path, "w");
  free(path); path = NULL;
  if (!notes) return false;

  bool ok = (fprintf(notes, "# %s\n\n\n", todo->name));
  fclose(notes);

  if (!ok) remove_todo_notes(todo);
  todo->notes = true;
  return ok;
}

Functionality todo_list_functionality[] = {
  { "add", "a", action_add_todo, MAN("Add a ToDo", "[name]") },
  { "add_at", "ap", action_add_at_todo, MAN("Add a ToDo in the specified position", "[index] [name]") },
  { "remove", "rm", action_remove_todo, MAN("Remove a ToDo", "[ID]") },
  { "move", "mv", action_move_todo, MAN("Move ToDo", "[ID]") },
  { "edit", "e", action_edit_todo, MAN("Edit a ToDo", "[ID] [new name]") },
  { "clear", NULL, action_clear_todos, MAN("Clear all ToDos", "all") },
  { "notes_remove", NULL, action_notes_todo_remove, MAN("Remove the notes file for the todo", "[ID]") },
};
unsigned int todo_list_functionality_count = sizeof(todo_list_functionality) / sizeof(Functionality);
