#ifndef CLI_H
#define CLI_H

#include <stdbool.h>

#include "parser.h"

#define TEMP_BUF_SIZE 512

#define DIFF_TOOL "nvim -d"

#define ANSI_RED "\033[0;31m"
#define ANSI_GREEN "\033[0;32m"
#define ANSI_BLUE "\033[0;34m"
#define ANSI_RESET "\033[0m"
#define DIFF_FORMAT "'" ANSI_BLUE "<<< changes\n" ANSI_GREEN "%>" ANSI_BLUE "===\n" ANSI_RED "%<" ANSI_BLUE ">>> local" ANSI_RESET "\n'"

Action_return print_todo(Input *input);
Action_return import_todo(Input *input);
Action_return export_todo(Input *input);
Action_return execute_commands(Input *input);
Action_return do_nothing(Input *input);

bool clone_text_file(char *origin_path, char *clone_path);

Action_return cli_parse_input(char *input);

#endif // CLI_H
