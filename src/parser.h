#ifndef ACTIONS_H
#define ACTIONS_H

typedef struct {
  char *input;
  unsigned int length;
  unsigned int cursor;
} Input;

typedef enum {
  RETURN_ERROR,
  RETURN_ERROR_AND_EXIT,
  RETURN_INFO,
  RETURN_SUCCESS,
} Action_return_type ;

typedef struct {
  char *message;
  char *file;
  unsigned int line;
  const char *function;
  Action_return_type type;
} Action_return;

#define ACTION_RETURN(return_type, return_message) (Action_return) { \
    .type = return_type,                                             \
    .message = return_message,                                       \
    .file = __FILE__,                                                \
    .line = __LINE__,                                                \
    .function = __func__,                                            \
  }

typedef struct {
  char *description;
  char **parameters; // Optional --> Empty if it doesn't expect parameters
} Manual;

// MAN(description of the command, [valid parameter combinations for this functionality])
#define MAN(description, ...) (Manual){ description, (char *[]){ __VA_ARGS__, NULL} }

typedef struct {
  char *full_cmd;         // Obligatory
  char *abbreviation_cmd; // Optional
  Action_return (*function_cmd)(Input *);
  Manual man;
} Functionality;

char *next_token(Input *cmd, char divider);

Action_return (*search_functionality_function(char *instruction, Functionality functionality[], unsigned int functionality_count))(Input *);

#endif
