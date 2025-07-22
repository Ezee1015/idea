#ifndef ACTIONS_H
#define ACTIONS_H

typedef struct {
  char *input;
  unsigned int input_length;
  unsigned int cursor;
} Cmd;

typedef enum {
  RETURN_ERROR,
  RETURN_ERROR_AND_EXIT,
  RETURN_INFO,
  RETURN_SUCCESS,
} Action_return_type ;

typedef struct {
  char *message;
  Action_return_type type;
} Action_return;

#define ACTION_RETURN(return_type, return_message) (Action_return) { \
  .type = return_type,                                               \
  .message = return_message                                          \
}

typedef struct {
  char *abbreviation_cmd;
  char *full_cmd;
  Action_return (*function_cmd)(Cmd *);
} Functionality;

char *next_token(Cmd *cmd, char divider);
Action_return action(char *input);

#endif
