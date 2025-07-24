#ifndef CLI_H
#define CLI_H

#include "parser.h"

Action_return print_todo(Input *input);
Action_return import_todo(Input *input);
Action_return export_todo(Input *input);

Action_return cli_parse_input(char *input);

#endif // CLI_H
