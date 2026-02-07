#ifndef CLI_H
#define CLI_H

#include <stdbool.h>

#include "parser.h"
#include "todo_list.h"

#define TEXT_EDITOR "nvim"
#define NOTES_ICON "󱅄" //     󰠮  󰺿  󰅏

#define DIFFTOOL_CMD "nvim -d"

#define ANSI_RED       (cli_disable_colors) ? "" : "\033[0;31m"
#define ANSI_GREEN     (cli_disable_colors) ? "" : "\033[0;32m"
#define ANSI_YELLOW    (cli_disable_colors) ? "" : "\033[0;33m"
#define ANSI_BLUE      (cli_disable_colors) ? "" : "\033[0;34m"
#define ANSI_GRAY      (cli_disable_colors) ? "" : "\033[0;90m"
#define ANSI_RESET     (cli_disable_colors) ? "" : "\033[0m"
#define ANSI_UNDERLINE (cli_disable_colors) ? "" : "\033[4m"

#define DIFF_FORMAT_FMT  "'%s<<< changes\n%s%%>%s===\n%s%%<%s>>> local%s\n'"
#define DIFF_FORMAT_ARGS ANSI_BLUE, ANSI_GREEN, ANSI_BLUE, ANSI_RED, ANSI_BLUE, ANSI_RESET
#define DIFF_CMD_FMT "diff --old-group-format=" DIFF_FORMAT_FMT      \
                         " --new-group-format=" DIFF_FORMAT_FMT      \
                         " --changed-group-format=" DIFF_FORMAT_FMT  \
                         " --unchanged-group-format='%%='"
#define DIFF_CMD_ARGS DIFF_FORMAT_ARGS, DIFF_FORMAT_ARGS, DIFF_FORMAT_ARGS

extern unsigned int msg_indentation;
#define PRINT(file, line, function, fmt, ...) printf("#%*s %s%s:%d (%s)%s: " fmt "\n",  msg_indentation, "|", ANSI_BLUE, file, line, function, ANSI_RESET, __VA_ARGS__)
#define PRINT_MESSAGE(fmt, ...) PRINT(__FILE__, __LINE__, __func__, fmt, __VA_ARGS__)
#define PRINT_TEXT(fmt) PRINT(__FILE__, __LINE__, __func__, fmt "%s", "")
#define NESTED_ACTION(operation, result) do {                                      \
    msg_indentation += 2;                                                          \
    operation;                                                                     \
    if (result.message && strcmp(result.message, ""))                              \
      PRINT(result.file, result.line, result.function, "[%s] %s", print_action_return(result.type), result.message); \
    msg_indentation -= 2;                                                          \
  } while (0)

extern bool cli_disable_colors;

Action_return action_print_todo(Input *input);
Action_return action_sync_todos(Input *input);
Action_return action_import_todos(Input *input);
Action_return action_export_todos(Input *input);
Action_return action_notes_todo(Input *input);
Action_return action_execute_commands(Input *input);
Action_return action_do_nothing(Input *input);
Action_return action_print_help(Input *input);
extern Functionality cli_functionality[];
extern unsigned int cli_functionality_count;

// Create a temporary file in the local directory to modify the notes of the ToDo
bool write_notes_to_file(Todo *todo);
bool load_notes_from_file(Todo *todo);

bool clone_text_file(char *origin_path, char *clone_path);

void print_todo(unsigned int index, Todo todo);

Action_return cli_parse_input(char *input);

char *print_action_return(Action_return_type type);

#endif // CLI_H
