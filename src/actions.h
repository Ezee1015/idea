#ifndef ACTIONS_H
#define ACTIONS_H

typedef struct {
  char *input;
  unsigned int input_length;
  unsigned int cursor;
} Cmd;

char *next_token(Cmd *cmd, char divider);

void action(char *input);

#endif
