#include <ncurses.h>
#include <stdlib.h>
#include <string.h>

#include "actions.h"
#include "db.h"
#include "idea.h"
#include "utils/list.h"

char *next_token(Cmd *cmd, char divider) {
  if (cmd->cursor > cmd->input_length) {
    INFO("Command malformed!");
    return NULL;
  }

  int i = cmd->cursor;
  while (i <= cmd->input_length && cmd->input[i] != divider) i++;

  unsigned int read = i - cmd->cursor;
  char *token = malloc(read + 1);
  memcpy(token, cmd->input + cmd->cursor, read);
  token[read] = '\0';

  cmd->cursor = i + 1;
  return token;
}

void action(char *input) {
  Cmd cmd = {
    .input = input,
    .input_length = strlen(input),
    .cursor = 0,
  };
  char *instruction = next_token(&cmd, ' ');

  if (!strcmp(instruction, CMD_ADD)) {
    Todo *new = malloc(sizeof(Todo));
    if (!new) return;
    new->data = next_token(&cmd, 0);
    if (!new->data) return;
    list_append(&todo_list, new);

  } else if (!strcmp(instruction, CMD_REMOVE)) {
    char *pos_str = next_token(&cmd, 0);
    if (!pos_str) return;
    unsigned int pos = atoi(pos_str);
    free(pos_str);

    if (pos == 0 || pos > list_size(todo_list)) INFO("Invalid Position");
    else free_todo(list_remove(&todo_list, pos-1));

  } else if (!strcmp(instruction, CMD_MOVE)) {
    char *pos_origin_str = next_token(&cmd, ' ');
    if (!pos_origin_str) return;
    char *pos_destination_str = next_token(&cmd, 0);
    if (!pos_destination_str) return;

    unsigned int pos_origin = atoi(pos_origin_str);
    unsigned int pos_destination = atoi(pos_destination_str);

    if (pos_origin == 0 || pos_origin > list_size(todo_list)) {
      INFO("Invalid origin position");
    } else if (pos_destination == 0 || pos_destination > list_size(todo_list)) {
      INFO("Invalid destination position");
    } else if (pos_destination == pos_origin) {
      INFO("Moving to the same position");
    } else {
      Todo *todo = list_remove(&todo_list, pos_origin-1);
      // if (pos_origin < pos_destination) pos_destination--;
      list_insert_at(&todo_list, todo, pos_destination-1);
    }

    free(pos_origin_str);
    free(pos_destination_str);

  } else if (!strcmp(instruction, CMD_EDIT)) {
    char *pos_str = next_token(&cmd, ' ');
    if (!pos_str) return;
    unsigned int pos = atoi(pos_str);

    if (pos == 0 || pos > list_size(todo_list)) {
      INFO("Invalid Position");
    } else {
      char *new_text = next_token(&cmd, 0);
      if (!new_text) {
        INFO("Empty new text.");
      } else {
        Todo *todo = list_get(todo_list, pos-1);
        free(todo->data);
        todo->data = new_text;
      }
    }
    free(pos_str);
  } else {
    INFO("Invalid command!");
  }

  free(instruction);
}
