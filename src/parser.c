#include <ncurses.h>
#include <stdlib.h>
#include <string.h>

#include "parser.h"

char *next_token(Input *input, char divider) {
  if (input->cursor > input->length) return NULL;

  unsigned int i = input->cursor;
  while (i <= input->length && input->input[i] != divider) i++;

  unsigned int read = i - input->cursor;
  if (read == 0) return NULL;
  char *token = malloc(read + 1);
  memcpy(token, input->input + input->cursor, read);
  token[read] = '\0';

  input->cursor = i + 1;
  return token;
}

Action_return (*search_functionality_function(char *instruction, Functionality functionality[], unsigned int functionality_count))(Input *input) {
  unsigned int i = 0;
  while (i < functionality_count) {
    if (functionality[i].abbreviation_cmd && !strcmp(instruction, functionality[i].abbreviation_cmd)) break;
    if (!strcmp(instruction, functionality[i].full_cmd)) break;
    i++;
  }
  return (i == functionality_count) ? NULL : functionality[i].function_cmd;
}
