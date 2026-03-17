#include <ncurses.h>
#include <string.h>

#include "parser.h"
#include "utils/string.h"

char *next_token(Input *input, char divider) {
  if (input->cursor > input->length) return NULL;

  String_builder sb = sb_new();
  unsigned int i = input->cursor;
  bool escaped = false;
  while (i <= input->length) {
    const char c = input->input[i];

    if (escaped) {
      if (c == '\\') {
        sb_append_char(&sb, c);
      } else if (c == divider) {
        sb_append_char(&sb, divider);
      } else {
        sb_append(&sb, (char[]){ '\\', c, '\0'});
      }
      escaped = false;
    } else {
      if (c == '\\') {
        escaped = true;
      } else {
        if (c == divider) break;
        sb_append_char(&sb, c);
      }
    }
    i++;
  }

  input->cursor = i + 1;
  return sb.str;
}

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
