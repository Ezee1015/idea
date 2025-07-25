#include <ncurses.h>
#include <stdlib.h>
#include <string.h>

#include "parser.h"

char *next_token(Input *cmd, char divider) {
  if (cmd->cursor > cmd->length) return NULL;

  unsigned int i = cmd->cursor;
  while (i <= cmd->length && cmd->input[i] != divider) i++;

  unsigned int read = i - cmd->cursor;
  char *token = malloc(read + 1);
  memcpy(token, cmd->input + cmd->cursor, read);
  token[read] = '\0';

  cmd->cursor = i + 1;
  return token;
}

Action_return (*search_functionality_pos(char *instruction, Functionality functionality[], unsigned int functionality_count))(Input *input) {
  unsigned int i = 0;
  while (i < functionality_count) {
    if (!strcmp(instruction, functionality[i].abbreviation_cmd)) break;
    if (!strcmp(instruction, functionality[i].full_cmd)) break;
    i++;
  }
  return (i == functionality_count) ? NULL : functionality[i].function_cmd;
}
