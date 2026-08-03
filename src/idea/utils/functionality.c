#include <ncurses.h>
#include <string.h>

#include "functionality.h"

bool (*search_functionality_function(char *instruction, Functionality functionality[], unsigned int functionality_count))(Input *input) {
  if (!instruction) return false;

  unsigned int i = 0;
  while (i < functionality_count) {
    if (functionality[i].abbreviation_cmd && !strcmp(instruction, functionality[i].abbreviation_cmd)) break;
    if (!strcmp(instruction, functionality[i].full_cmd)) break;
    i++;
  }
  return (i == functionality_count) ? NULL : functionality[i].function_cmd;
}

bool action_do_nothing(Input *input) {
  input->cursor = input->length+1;
  return true;
}
