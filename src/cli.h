#ifndef CLI_H
#define CLI_H

#include <stdbool.h>

#include "parser.h"
#include "todo_list.h"

#define NOTES_ICON "󱅄" //     󰠮  󰺿  󰅏

// Box chars source: <https://en.wikipedia.org/wiki/Box-drawing_characters>
#define BOX_V_BAR "│"
#define BOX_H_BAR "─"
#define BOX_T "┬"

#define TEXT_EDITOR "nvim"
#define DIFFTOOL_CMD "nvim -d"

#define ANSI_RED          (cli_disable_colors) ? "" : "\033[0;31m"
#define ANSI_GREEN        (cli_disable_colors) ? "" : "\033[0;32m"
#define ANSI_YELLOW       (cli_disable_colors) ? "" : "\033[0;33m"
#define ANSI_BLUE         (cli_disable_colors) ? "" : "\033[0;34m"
#define ANSI_GRAY         (cli_disable_colors) ? "" : "\033[0;90m"
#define ANSI_RESET        (cli_disable_colors) ? "" : "\033[0m"
#define ANSI_UNDERLINE    (cli_disable_colors) ? "" : "\033[4m"
#define ANSI_CLEAR_SCREEN (cli_disable_colors) ? "" : "\033[2J"

#define DIFF_FORMAT_FMT  "'%s<<< changes\n%s%%>%s===\n%s%%<%s>>> local%s\n'"
#define DIFF_FORMAT_ARGS ANSI_BLUE, ANSI_GREEN, ANSI_BLUE, ANSI_RED, ANSI_BLUE, ANSI_RESET
#define DIFF_CMD_FMT "diff --old-group-format=" DIFF_FORMAT_FMT      \
                         " --new-group-format=" DIFF_FORMAT_FMT      \
                         " --changed-group-format=" DIFF_FORMAT_FMT  \
                         " --unchanged-group-format='%%='"
#define DIFF_CMD_ARGS DIFF_FORMAT_ARGS, DIFF_FORMAT_ARGS, DIFF_FORMAT_ARGS

extern bool cli_disable_colors;

void cli_print_backtrace();

bool action_print_new_line(Input *input);
bool action_list_todos(Input *input);
bool action_reminders(Input *input);
bool action_sync_todos(Input *input);
bool action_import_todos(Input *input);
bool action_export_todos(Input *input);
bool action_notes_todo(Input *input);
bool action_print_notes(Input *input);
bool action_execute_commands(Input *input);
bool action_print_help(Input *input);
bool action_loop(Input *input);
extern Functionality cli_functionality[];
extern unsigned int cli_functionality_count;

// Create a temporary file in the local directory to modify the notes of the ToDo
bool write_notes_to_temporal_file(Todo *todo);
bool load_notes_from_temporal_file(Todo *todo);

bool clone_text_file(char *origin_path, char *clone_path);

bool cli_parse_input(char *input);

#endif // CLI_H
