#include <ncurses.h>
#include <stdlib.h>
#include <string.h>

#include "actions.h"
#include "db.h"
#include "idea.h"

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
    Todo new = {0};
    new.data = next_token(&cmd, 0);
    if (!new.data) return;
    if (!create_todo_node(new)) ERROR_AND_EXIT("Can't save the new todo!");

  } else if (!strcmp(instruction, CMD_REMOVE)) {
    char *pos_str = next_token(&cmd, 0);
    if (!pos_str) return;

    Todo_list *node;
    unsigned int pos = atoi(pos_str);
    if (pos == 0 || !(node = remove_todo_node(pos))) INFO("Invalid Position");
    else free_todo_node(node);
    free(pos_str);

  } else if (!strcmp(instruction, CMD_MOVE)) {
    char *pos_origin_str = next_token(&cmd, ' ');
    if (!pos_origin_str) return;
    char *pos_destination_str = next_token(&cmd, 0);
    if (!pos_destination_str) return;

    unsigned int pos_origin = atoi(pos_origin_str);
    unsigned int pos_destination = atoi(pos_destination_str);

    if (pos_origin == 0 || pos_destination == 0 || !move_todo_node(pos_origin, pos_destination))
      INFO("Invalid Position");

    free(pos_origin_str);
    free(pos_destination_str);

  } else if (!strcmp(instruction, CMD_EDIT)) {
    char *pos_str = next_token(&cmd, ' ');
    if (!pos_str) return;

    Todo_list *node;
    unsigned int pos = atoi(pos_str);
    if (pos == 0 || !(node = get_node(pos))) {
      INFO("Invalid Position");
    } else {
      char *new_text = next_token(&cmd, 0);
      if (!new_text) {
        ERROR_AND_EXIT("EMPTY NEW TEXT!");
      } else {
        free(node->todo.data);
        node->todo.data = new_text;
      }
    }
    free(pos_str);
  } else {
    INFO("Invalid command!");
  }

  free(instruction);
}
