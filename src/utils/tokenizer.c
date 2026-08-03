#include "tokenizer.h"
#include "string.h"

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

bool has_more_tokens(Input *input, char **left_overs) {
  bool has_left = input->cursor <= input->length;
  if (left_overs && has_left) *left_overs = input->input + input->cursor;
  return has_left;
}
