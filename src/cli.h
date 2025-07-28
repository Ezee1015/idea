#ifndef CLI_H
#define CLI_H

#include <stdbool.h>

#include "parser.h"
#include "todo_list.h"

#define TEMP_BUF_SIZE 512

#define DIFFTOOL_CMD "nvim -d"

#define ANSI_RED "\033[0;31m"
#define ANSI_GREEN "\033[0;32m"
#define ANSI_BLUE "\033[0;34m"
#define ANSI_RESET "\033[0m"
#define DIFF_FORMAT "'" ANSI_BLUE "<<< changes\n" ANSI_GREEN "%>" ANSI_BLUE "===\n" ANSI_RED "%<" ANSI_BLUE ">>> local" ANSI_RESET "\n'"
#define DIFF_CMD "diff --old-group-format=" DIFF_FORMAT      \
                     " --new-group-format=" DIFF_FORMAT      \
                     " --changed-group-format=" DIFF_FORMAT  \
                     " --unchanged-group-format='%='"

extern unsigned int msg_indentation;
#define PRINT(file, line, function, fmt, ...) printf("#%*s " ANSI_BLUE "%s:%d (%s)" ANSI_RESET ": " fmt "\n",  msg_indentation, "|", file, line, function, __VA_ARGS__)
#define PRINT_MESSAGE(fmt, ...) PRINT(__FILE__, __LINE__, __func__, fmt, __VA_ARGS__)
#define NESTED_ACTION(operation, result) do {                                      \
    msg_indentation += 2;                                                          \
    operation;                                                                     \
    if (result.message && strcmp(result.message, ""))                              \
      PRINT(result.file, result.line, result.function, "[%s] %s", print_action_return(result.type), result.message); \
    msg_indentation -= 2;                                                          \
  } while (0)

Action_return action_print_todo(Input *input);
Action_return action_import_todo(Input *input);
Action_return action_export_todo(Input *input);
Action_return action_execute_commands(Input *input);
Action_return action_do_nothing(Input *input);
Action_return action_print_help(Input *input);
extern Functionality cli_functionality[];
extern unsigned int cli_functionality_count;

void print_todo(unsigned int index, Todo todo);

bool clone_text_file(char *origin_path, char *clone_path);

Action_return cli_parse_input(char *input);

char *print_action_return(Action_return_type type);

#endif // CLI_H
