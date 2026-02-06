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
      switch (c) {
        case '\\':
          if (c == divider) break;
          sb_append_char(&sb, c);
          escaped = false;
          break;

        case ' ':
          sb_append_char(&sb, ' ');
          escaped = false;
          break;

        default:
          if (c == divider) break;
          sb_append(&sb, (char[]){ '\\', c, '\0'});
          escaped = false;
          break;
      }
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

Action_return (*search_functionality_function(char *instruction, Functionality functionality[], unsigned int functionality_count))(Input *input) {
  unsigned int i = 0;
  while (i < functionality_count) {
    if (functionality[i].abbreviation_cmd && !strcmp(instruction, functionality[i].abbreviation_cmd)) break;
    if (!strcmp(instruction, functionality[i].full_cmd)) break;
    i++;
  }
  return (i == functionality_count) ? NULL : functionality[i].function_cmd;
}
