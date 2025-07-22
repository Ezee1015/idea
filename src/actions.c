#include <ncurses.h>
#include <stdlib.h>
#include <string.h>

#include "actions.h"
#include "db.h"
#include "utils/list.h"

/// FUNCTIONALITY

Action_return add_todo(Cmd *cmd) {
  Todo *new = malloc(sizeof(Todo));
  if (!new) return ACTION_RETURN(RETURN_ERROR_AND_EXIT, "No more memory");
  new->data = next_token(cmd, 0);
  if (!new->data) return ACTION_RETURN(RETURN_ERROR, "Command malformed");
  list_append(&todo_list, new);
  return ACTION_RETURN(RETURN_SUCCESS, "");
}

Action_return remove_todo(Cmd *cmd) {
  char *pos_str = next_token(cmd, 0);
  if (!pos_str) return ACTION_RETURN(RETURN_ERROR, "Command malformed");
  unsigned int pos = atoi(pos_str);
  free(pos_str);

  if (pos == 0 || pos > list_size(todo_list)) return ACTION_RETURN(RETURN_ERROR, "Invalid Position");

  free_todo(list_remove(&todo_list, pos-1));
  return ACTION_RETURN(RETURN_SUCCESS, "");
}

Action_return move_todo(Cmd *cmd) {
  char *pos_origin_str = next_token(cmd, ' ');
  if (!pos_origin_str) return ACTION_RETURN(RETURN_ERROR, "Command malformed");
  unsigned int pos_origin = atoi(pos_origin_str);
  free(pos_origin_str);

  char *pos_destination_str = next_token(cmd, 0);
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

Action_return edit_todo(Cmd *cmd) {
    char *pos_str = next_token(cmd, ' ');
    if (!pos_str) return ACTION_RETURN(RETURN_ERROR, "Command malformed");
    unsigned int pos = atoi(pos_str);
    free(pos_str);

    if (pos == 0 || pos > list_size(todo_list)) return ACTION_RETURN(RETURN_ERROR, "Invalid Position");

    char *new_text = next_token(cmd, 0);
    if (!new_text) return ACTION_RETURN(RETURN_ERROR, "Empty new text.");

    Todo *todo = list_get(todo_list, pos-1);
    free(todo->data);
    todo->data = new_text;
    return ACTION_RETURN(RETURN_SUCCESS, "");
}

Functionality functionality[] = {
  { "a", "add", add_todo },
  { "rm", "" , remove_todo },
  { "mv" , "move", move_todo },
  { "e" , "edit", edit_todo },
};

/// PARSING

char *next_token(Cmd *cmd, char divider) {
  if (cmd->cursor > cmd->input_length) return NULL;

  int i = cmd->cursor;
  while (i <= cmd->input_length && cmd->input[i] != divider) i++;

  unsigned int read = i - cmd->cursor;
  char *token = malloc(read + 1);
  memcpy(token, cmd->input + cmd->cursor, read);
  token[read] = '\0';

  cmd->cursor = i + 1;
  return token;
}

Action_return action(char *input) {
  Cmd cmd = {
    .input = input,
    .input_length = strlen(input),
    .cursor = 0,
  };
  char *instruction = next_token(&cmd, ' ');

  unsigned int functionality_count = sizeof(functionality) / sizeof(Functionality);
  int i = 0;
  while (i < functionality_count) {
    if (!strcmp(instruction, functionality[i].abbreviation_cmd)) break;
    if (!strcmp(instruction, functionality[i].full_cmd)) break;
    i++;
  }
  free(instruction); instruction = NULL;

  if (i == functionality_count) return ACTION_RETURN(RETURN_ERROR, "Invalid command");

  Action_return action_return = functionality[i].function_cmd(&cmd);
  switch (action_return.type) {
    case RETURN_INFO:
    case RETURN_SUCCESS:
      if (cmd.cursor <= cmd.input_length) abort(); // Command parsing error. There's data left in the command
      todo_list_modified = true;
      break;

    case RETURN_ERROR:
    case RETURN_ERROR_AND_EXIT:
      break;
  }

  return action_return;
}
