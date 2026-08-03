#ifndef FUNCTIONALITY_H
#define FUNCTIONALITY_H

#include "../../utils/tokenizer.h"

#define ACTION_NO_ARGS(action_name, input) do {                                          \
  if (has_more_tokens(input, NULL)) {                                                    \
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

bool (*search_functionality_function(char *instruction, Functionality functionality[], unsigned int functionality_count))(Input *);
bool action_do_nothing(Input *input);

#endif
