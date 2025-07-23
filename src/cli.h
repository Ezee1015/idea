#ifndef CLI_H
#define CLI_H

#include "parser.h"

Action_return print_todo(Input *input);

void cli_parse_input(int argc, char *argv[]);

#endif // CLI_H
