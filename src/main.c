#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "main.h"
#include "parser.h"
#include "todo_list.h"
#include "tui.h"
#include "cli.h"
#include "utils/list.h"
#include "utils/string.h"

State idea_state = {0};
List backtrace = {0};

List todo_list = list_new();
bool todo_list_modified = false;

bool lock_file() {
  if (access(idea_state.lock_filepath, F_OK) == 0) {
    APPEND_TO_BACKTRACE(BACKTRACE_ERROR, "Idea is already running...");
    return false;
  }

  FILE *lock_file = fopen(idea_state.lock_filepath, "w");
  if (!lock_file) {
    APPEND_TO_BACKTRACE(BACKTRACE_ERROR, "Error creating the lock...");
    return false;
  }

  fclose(lock_file);
  return true;
}

bool unlock_file() {
  bool removed = (remove(idea_state.lock_filepath) == 0);
  if (!removed) APPEND_TO_BACKTRACE(BACKTRACE_ERROR, "Unable to remove the lock file");
  return removed;
}

Date date_now() {
  time_t now       = time(NULL);
  struct tm *now_t = localtime(&now);

  return (Date) {
    .year  = now_t->tm_year + 1900,
    .month = now_t->tm_mon + 1,
    .day   = now_t->tm_mday,
  };
}

int get_delta_time_days(Date date_from, Date date_to) {
  // Source: <https://stackoverflow.com/a/73846054>
  struct tm date_from_tm = {
    .tm_year = date_from.year - 1900,
    .tm_mon = date_from.month - 1,
    .tm_mday = date_from.day,
    .tm_isdst = -1,
  };

  struct tm date_to_tm = {
    .tm_year = date_to.year - 1900,
    .tm_mon = date_to.month - 1,
    .tm_mday = date_to.day,
    .tm_isdst = -1,
  };

  time_t date_from_t = mktime(&date_from_tm);
  time_t date_to_t = mktime(&date_to_tm);

  double dt = difftime(date_to_t, date_from_t);
  int days = dt / 86400;

  return days;
}

char *get_delta_time_string(Date date_from, Date date_to) {
  int days = get_delta_time_days(date_from, date_to);
  char *when = (days < 0) ? "ago" : "left";
  days = abs(days);

  if (days == 0) return sb_create("0 days left").str;

  int y = days/365; // It floors it automatically
  int d_of_y = y * 365; // Days from the amount of years

  int m = (days - d_of_y)/30;
  int d_of_m = m * 30;

  int w = (days - d_of_y - d_of_m)/7;
  int d_of_w = w*7;

  int d = days - d_of_y - d_of_m - d_of_w;

  String_builder sb = sb_new();

  if (y) sb_append_with_format(&sb, "%d year%s ", y, (y != 1) ? "s" : "");
  if (m) sb_append_with_format(&sb, "%d month%s ", m, (m != 1) ? "s" : "");
  if (w) sb_append_with_format(&sb, "%d week%s ", w, (w != 1) ? "s" : "");
  if (d) sb_append_with_format(&sb, "%d day%s ", d, (d != 1) ? "s" : "");
  sb_append(&sb, when);

  return sb.str;
}

bool is_date_equals(Date date_1, Date date_2) {
  return (date_1.year == date_2.year && date_1.month == date_2.month && date_1.day == date_2.day);
}

bool is_date_greater(Date date_greater, Date date_less) {
  return get_delta_time_days(date_less, date_greater) > 0;
}

bool is_date_less(Date date_less, Date date_greater) {
  return is_date_greater(date_greater, date_less);
}

bool load_date_from_string(char *date_str, Date *date) {
  if (!date_str || !date) return false;

  Input date_input = {
    .input = date_str,
    .length = strlen(date_str),
    .cursor = 0,
  };

  // Year
  char *rem_year_str = next_token(&date_input, '-');
  if (!rem_year_str) {
    APPEND_TO_BACKTRACE(BACKTRACE_ERROR, "Unable to get the year from the date '%s'", date_str);
    return false;
  }

  date->year = atoi(rem_year_str);
  free(rem_year_str);
  if (date->year == 0) {
    APPEND_TO_BACKTRACE(BACKTRACE_ERROR, "Unable to parse the year from the date '%s'", date_str);
    return false;
  }

  // Month
  char *rem_month_str = next_token(&date_input, '-');
  if (!rem_month_str) {
    APPEND_TO_BACKTRACE(BACKTRACE_ERROR, "Unable to get the month from the date '%s'", date_str);
    return false;
  }

  date->month = atoi(rem_month_str);
  free(rem_month_str);
  if (date->month == 0) {
    APPEND_TO_BACKTRACE(BACKTRACE_ERROR, "Unable to parse the month from the date '%s'", date_str);
    return false;
  }

  // day
  char *rem_day_str = next_token(&date_input, '-');
  if (!rem_day_str) {
    APPEND_TO_BACKTRACE(BACKTRACE_ERROR, "Unable to get the day from the date '%s'", date_str);
    return false;
  }

  date->day = atoi(rem_day_str);
  free(rem_day_str);
  if (date->day == 0) {
    APPEND_TO_BACKTRACE(BACKTRACE_ERROR, "Unable to parse the day from the date '%s'", date_str);
    return false;
  }

  return true;
}

bool parse_commands_cli(char *commands[], int count) {
  if (count == 0) return false;

  int i=0;
  bool something_went_wrong = false;

  cli_disable_colors = getenv("IDEA_CLI_DISABLE_COLORS");

  if (!strcmp(commands[0], "-m")) {
    count--;
    commands++;

    while (!something_went_wrong && i<count) {
      bool result = cli_parse_input(commands[i]);

      if (result) {
        if (!list_is_empty(backtrace)) {
            APPEND_TO_BACKTRACE(BACKTRACE_INFO, "Message from the %dº command (%s)", i+1, commands[i]);
        }
      } else {
        APPEND_TO_BACKTRACE(BACKTRACE_ERROR, "An ERROR occurred in the %dº command (%s). Idea is not saving the changes made in this instance", i+1, commands[i]);
        todo_list_modified = false; // Try to not save it because it may be corrupted
        something_went_wrong = true;
      }
      cli_print_backtrace();

      i++;
    }
  } else {
    String_builder input = sb_new();

    while (i<count) {
      String_builder arg = sb_create("%s", commands[i]);
      sb_search_and_replace(&arg, " ", "\\ ");
      sb_append(&input, arg.str);
      sb_free(&arg);
      if (i < count-1) sb_append_char(&input, ' ');
      i++;
    }

    bool result = cli_parse_input(input.str);

    if (result) {
      if (!list_is_empty(backtrace)) {
          APPEND_TO_BACKTRACE(BACKTRACE_INFO, "Message from the command '%s'", input.str);
      }
    } else {
      APPEND_TO_BACKTRACE(BACKTRACE_ERROR, "An ERROR occurred in the command '%s'. Idea is not saving the changes made in this instance", input.str);
      todo_list_modified = false; // Try to not save it because it may be corrupted
      something_went_wrong = true;
    }

    sb_free(&input);
    cli_print_backtrace();
  }

  if (todo_list_modified) action_list_todos(NULL);
  return !something_went_wrong;
}

bool load_paths() {
  String_builder sb = sb_new();
  if (!sb_append_from_shell_variable(&sb, "TMPDIR")) sb_append(&sb, "/tmp");
  idea_state.tmp_path = sb.str;

  sb = sb_new();
  if (sb_append_from_shell_variable(&sb, "IDEA_LOCAL_PATH")) {
    if (sb.str[sb.length-1] == '/') {
      APPEND_TO_BACKTRACE(BACKTRACE_ERROR, "IDEA_LOCAL_PATH can't end with '/'");
      return false;
    }
  } else {
    if (!sb_append_from_shell_variable(&sb, "HOME")) {
      APPEND_TO_BACKTRACE(BACKTRACE_ERROR, "Set the HOME environment variable...");
      return false;
    }
    sb_append(&sb, "/" LOCAL_PATH);
  }
  idea_state.local_path = sb.str;

  idea_state.todos_filepath = sb_create("%s/" SAVE_FILENAME, idea_state.local_path).str;
  idea_state.lock_filepath = sb_create("%s/" LOCK_FILENAME, idea_state.local_path).str;

  sb = sb_new();
  if (sb_append_from_shell_variable(&sb, "IDEA_CONFIG_PATH")) {
    if (sb.str[sb.length-1] == '/') {
      APPEND_TO_BACKTRACE(BACKTRACE_ERROR, "IDEA_CONFIG_PATH can't end with '/'");
      return false;
    }
  } else {
    if (!sb_append_from_shell_variable(&sb, "HOME")) {
      APPEND_TO_BACKTRACE(BACKTRACE_ERROR, "Set the HOME environment variable...");
      return false;
    }
    sb_append(&sb, "/" CONFIG_PATH);
  }
  sb_append(&sb, "/" CONFIG_FILENAME);
  idea_state.config_filepath = sb.str;

  return true;
}

void free_paths() {
  if (idea_state.tmp_path) free(idea_state.tmp_path);
  if (idea_state.lock_filepath) free(idea_state.lock_filepath);
  if (idea_state.todos_filepath) free(idea_state.todos_filepath);
  if (idea_state.config_filepath) free(idea_state.config_filepath);
  if (idea_state.local_path) free(idea_state.local_path);

  free_config(idea_state.config);
}

void free_backtrace_item(Backtrace_item *b) {
  free(b->message);
  free(b);
}

int main(int argc, char *argv[]) {
  enum {
    RET_CODE_SUCCESS,
    RET_CODE_PATH_ERROR,
    RET_CODE_CONFIG_ERROR,
    RET_CODE_CREATE_STRUCTURE_FAILED,
    RET_CODE_LOCK_ERROR,
    RET_CODE_LOAD_FILE_ERROR,
    RET_CODE_SAVE_FILE_ERROR,
    RET_CODE_UNLOCK_ERROR,
    RET_CODE_TUI_ERROR,
    RET_CODE_CLI_ERROR,
  } ret;

  idea_state.program_path = argv[0];
  if (!load_paths()) {
    free_paths();
    APPEND_TO_BACKTRACE(BACKTRACE_ERROR, "Unable to load the paths");
    cli_print_backtrace();
    return RET_CODE_PATH_ERROR;
  }

  if (!load_config()) {
    free_paths();
    APPEND_TO_BACKTRACE(BACKTRACE_ERROR, "Unable to load the config");
    cli_print_backtrace();
    return RET_CODE_CONFIG_ERROR;
  }

  if (!create_dir_structure()) {
    free_paths();
    APPEND_TO_BACKTRACE(BACKTRACE_ERROR, "Unable to create the directory structure");
    cli_print_backtrace();
    return RET_CODE_CREATE_STRUCTURE_FAILED;
  }

  if (!lock_file())  {
    free_paths();
    APPEND_TO_BACKTRACE(BACKTRACE_ERROR, "Unable to create the lock file");
    cli_print_backtrace();
    return RET_CODE_LOCK_ERROR;
  }

  if (load_todo_list(&todo_list, idea_state.todos_filepath, false)) {
    if (argc == 1) {
      // TUI Version
      ret = (window_app()) ? RET_CODE_SUCCESS : RET_CODE_TUI_ERROR;
    } else {
      // CLI Version
      argv++; argc--;
      ret = (parse_commands_cli(argv, argc)) ? RET_CODE_SUCCESS : RET_CODE_CLI_ERROR;
    }
  } else {
    ret = RET_CODE_LOAD_FILE_ERROR;
  }

  if (ret == RET_CODE_SUCCESS && todo_list_modified) {
    if (!save_todo_list(todo_list, idea_state.todos_filepath)) {
      APPEND_TO_BACKTRACE(BACKTRACE_ERROR, "Unable to save the ToDos in '%s'", idea_state.todos_filepath);
      cli_print_backtrace();
      ret = RET_CODE_SAVE_FILE_ERROR;
    }
  }

  if (!unlock_file()) {
    APPEND_TO_BACKTRACE(BACKTRACE_ERROR, "Unable to remove the lock file");
    cli_print_backtrace();
    ret = RET_CODE_UNLOCK_ERROR;
  }

  list_destroy(&todo_list, (void (*)(void *))free_todo);
  free_paths();
  cli_print_backtrace();
  return ret;
}
