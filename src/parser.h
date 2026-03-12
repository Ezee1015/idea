#ifndef ACTIONS_H
#define ACTIONS_H

#include "main.h"

typedef struct {
  char *input;
  unsigned int length;
  unsigned int cursor;
} Input;

#define ACTION_NO_ARGS(action_name, input) do {                                          \
  char *args;                                                                            \
  if ( input && (args = next_token(input, 0)) ) {                                        \
    free(args);                                                                          \
    APPEND_TO_BACKTRACE(BACKTRACE_ERROR, "`" action_name "` doesn't require arguments"); \
    return false;                                                                        \
  }                                                                                      \
} while (0)

typedef struct {
  char *description;
  char **parameters; // Optional --> Empty if it doesn't expect parameters
} Manual;

// MAN(description of the command, [valid parameter combinations for this functionality])
#define MAN(description, ...) (Manual){ description, (char *[]){ __VA_ARGS__, NULL} }

typedef struct {
  char *full_cmd;         // Obligatory
  char *abbreviation_cmd; // Optional
  bool (*function_cmd)(Input *);
  Manual man;
} Functionality;

char *next_token(Input *input, char divider);

bool (*search_functionality_function(char *instruction, Functionality functionality[], unsigned int functionality_count))(Input *);
bool action_do_nothing(Input *input);

#endif
