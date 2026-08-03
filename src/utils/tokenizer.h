#ifndef TOKENIZER_H
#define TOKENIZER_H

#include <stdbool.h>

typedef struct {
  char *input;
  unsigned int length;
  unsigned int cursor;
} Input;

char *next_token(Input *input, char divider);
bool has_more_tokens(Input *input, char **left_overs);

#endif // TOKENIZER_H
