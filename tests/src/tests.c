#include <stdio.h>
#include <stdbool.h>
#include <malloc.h>
#include <dirent.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <sys/stat.h>

#include "../../src/cli.h"
#include "../../src/parser.h"
#include "../../src/todo_list.h"

#include "tests.h"

Tests_state state = {
  .idea_config_path = NULL,
  .idea_export_path = NULL,
  .idea_path = "build/idea",

  .repo_path = NULL,

  .tests_filepath = NULL,
  .initial_states_path = NULL,
  .final_states_path = NULL,
  .logs_path = NULL,

  .log = false,
};

// Lines of the export/state file that should not be compared,
// because they are unique to the moment the program is run
const char *incomparable_lines[] = {
  EXPORT_FILE_INDENTATION "id:",
};
const unsigned int incomparable_lines_count = sizeof(char *) / sizeof(incomparable_lines);

void free_test(Test *test) {
  if (test->name) free(test->name);
  if (test->state) free(test->state);
  list_destroy(&test->instructions, free);
  free(test);
}

bool create_dir_if_not_exists(char *dir_path) {
  struct stat st = {0};
  if (stat(dir_path, &st) != -1) return (S_ISDIR(st.st_mode));
  return (mkdir(dir_path, 0755) == 0);
}

bool is_a_valid_test(Test *test) {
  if (!test->name)  {
    printf("[ERROR] The test has no name!\n");
    return false;
  }

  if (!test->state)  {
    printf("[ERROR] The initial state of the test '%s' was expected, but not found!\n", test->name);
    return false;
  }

  return true;
}

bool get_tests(List *tests) {
  bool ret = true;

  FILE *tests_file = fopen(state.tests_filepath, "r");
  if (!tests_file) {
    printf("Unable to open %s\n", state.tests_filepath);
    ret = false;
    goto exit;
  }

  Test *test = NULL;
  String_builder line = str_new();
  unsigned int line_nr = 1;
  while (str_read_line(tests_file, &line)) {
    if (str_is_empty(line)) {
      if (test && !is_a_valid_test(test)) {
        ret = false;
        goto exit;
      }

      test = NULL;
      line_nr++;
      continue;
    }

    Input input_line = {
      .input = str_to_cstr(line),
      .cursor = 0,
      .length = str_length(line)
    };
    char *tag = next_token(&input_line, ' ');

    if (!strcmp(tag, "--")) {
      free(tag);
      // Do nothing

    } else if (!strcmp(tag, "name:")) {
      free(tag);

      if (test) {
        printf("%s:%d: [ERROR] Name already provided!\n", state.tests_filepath, line_nr);
        ret = false;
        goto exit;
      }

      test = malloc(sizeof(Test));
      memset(test, 0 , sizeof(Test));
      test->name = next_token(&input_line, 0);
      list_append(tests, test);

    } else if (!strcmp(tag, "initial_state:")) {
      free(tag);

      if (!test) {
        printf("%s:%d: [ERROR] Name of the test is not provided!\n", state.tests_filepath, line_nr);
        ret = false;
        goto exit;
      }

      if (test->state) {
        printf("%s:%d: [ERROR] State already provided!\n", state.tests_filepath, line_nr);
        ret = false;
        goto exit;
      }

      test->state = next_token(&input_line, 0);

    } else if (!strcmp(tag, "command:")) {
      free(tag);

      if (!test) {
        printf("%s:%d: [ERROR] Name of the test is not provided!\n", state.tests_filepath, line_nr);
        ret = false;
        goto exit;
      }

      list_append(&test->instructions, next_token(&input_line, 0));

    } else if (!strcmp(tag, "return:")) {
      free(tag);

      if (!test) {
        printf("%s:%d: [ERROR] Name of the test is not provided!\n", state.tests_filepath, line_nr);
        ret = false;
        goto exit;
      }

      char *return_value_str = next_token(&input_line, 0);
      test->expected_return = atoi(return_value_str);
      if (test->expected_return) test->expect_state_unchanged = true;
      free(return_value_str);

    } else if (!strcmp(tag, "state_unchanged")) {
      free(tag);

      if (!test) {
        printf("%s:%d: [ERROR] Name of the test is not provided!\n", state.tests_filepath, line_nr);
        ret = false;
        goto exit;
      }

      char *params = next_token(&input_line, 0);
      if (params) {
        free(params);
        printf("%s:%d: [ERROR] state_unchanged is a property, you shouldn't pass any parameters to it!\n", state.tests_filepath, line_nr);
        ret = false;
        goto exit;
      }

      test->expect_state_unchanged = true;

    } else if (!strcmp(tag, "command:")) {
      free(tag);

      if (!test) {
        printf("%s:%d: [ERROR] Name of the test is not provided!\n", state.tests_filepath, line_nr);
        ret = false;
        goto exit;
      }

      list_append(&test->instructions, next_token(&input_line, 0));

    } else {
      printf("%s:%d: Not recognized '%s'\n", state.tests_filepath, line_nr, tag);
      free(tag);
      ret = false;
      goto exit;
    }

    str_clean(&line);
    line_nr++;
  }

  if (test && !is_a_valid_test(test)) ret = false;

exit:
  str_free(&line);
  if (tests_file) fclose(tests_file);
  return ret;
}

char *result_to_cstr(Result r) {
  switch (r) {
    case RESULT_NOT_TESTED: return ANSI_YELLOW CASE_UNTESTED ANSI_RESET;
    case RESULT_PASSED: return ANSI_GREEN CASE_PASSED ANSI_RESET;
    case RESULT_FAILED: return ANSI_RED CASE_FAILED ANSI_RESET;
    case RESULT_NOT_SPECIFIED: return ANSI_GRAY CASE_NOT_SPECIFIED ANSI_RESET;
  }
  return NULL;
}

void print_results_header(unsigned int test_length) {
  char *results_name[] = {
    "Import state",
    "Expected Return",
    "Expected State",
    "Clear"
  };

  // From https://github.com/bext-lang/b/blob/c6a21ba4c87ff1c304543c74dd910de34edad445/src/btest.rs#L228
  for (unsigned int i = 0; i < sizeof(results_name)/sizeof(char *); i++) {
      char *test_name = results_name[i];
      printf("%*s", test_length + CASE_LENGTH / 2 + 1, "");
      for (unsigned int x = 0; x < i; x++) printf("│%*s", CASE_LENGTH, "");
      printf("┌─ %s\n", test_name);
  }
}

void print_results(Test test, unsigned int test_name_length) {
  printf("%*s %s %s %s %s\n",
         test_name_length, test.name,
         result_to_cstr(test.results.state_applied),
         result_to_cstr(test.results.expected_return),
         result_to_cstr(test.results.expected_state),
         result_to_cstr(test.results.clear_after_test));
}

char *run_test_generate_base_command() {
  String_builder base_cmd = str_new();
  str_append(&base_cmd, "IDEA_CONFIG_PATH=\"");
  str_append(&base_cmd, state.idea_config_path);
  str_append(&base_cmd, "\" ");
  str_append(&base_cmd, state.repo_path);
  str_append_to_path(&base_cmd, state.idea_path);
  str_append(&base_cmd, " ");
  return str_to_cstr(base_cmd);
}

bool run_test_execute(char *test_name, String_builder *cmd, int *ret) {
  if (state.log) {
    String_builder log_path = str_new();
    str_append(&log_path, state.logs_path);
    str_append_to_path(&log_path, test_name);
    str_append(&log_path, ".txt");
    FILE *log = fopen(str_to_cstr(log_path), "a");
    if (!log) {
      printf("Unable to open log: %s!\n", str_to_cstr(log_path));
      str_free(&log_path);
      return false;
    }
    fprintf(log, "-----> Running: %s\n", str_to_cstr(*cmd));
    fclose(log);

    str_append(cmd, " >> \"");
    str_append_str(cmd, log_path);
    str_append(cmd, "\"");
    str_free(&log_path);
  } else {
    str_append(cmd, " > /dev/null");
  }

  int system_ret = system(str_to_cstr(*cmd));
  if (system_ret == -1 || !WIFEXITED(system_ret)) return false;
  *ret = WEXITSTATUS(system_ret);
  return true;
}

bool run_test_import_state(char *base_cmd, Test *test) {
  String_builder cmd = str_new();
  str_append(&cmd, base_cmd);
  str_append(&cmd, "\"import_no_diff ");
  str_append(&cmd, state.initial_states_path);
  str_append_to_path(&cmd, test->state);
  str_append(&cmd, ".idea\"");

  int cmd_ret;
  bool ok = (run_test_execute(test->name, &cmd, &cmd_ret) && cmd_ret == 0);
  str_free(&cmd);
  return ok;
}

bool run_test_generate_command(char *base_cmd, Test *test, String_builder *cmd) {
  str_append(cmd, base_cmd);
  if (list_is_empty(test->instructions)) return false;

  List_iterator iterator = list_iterator_create(test->instructions);
  while (list_iterator_next(&iterator)) {
    char *instruction = list_iterator_element(iterator);

    str_append(cmd, "\"");
    str_append(cmd, instruction);
    str_append(cmd, "\" ");
  }

  return true;
}

bool run_test_export_state(char *test_name, char *base_cmd) {
  String_builder cmd = str_new();
  str_append(&cmd, base_cmd);
  str_append(&cmd, "\"export ");
  str_append(&cmd, state.idea_export_path);
  str_append(&cmd, "\" ");

  int cmd_ret;
  bool ok = (run_test_execute(test_name, &cmd, &cmd_ret) && cmd_ret == 0);
  str_free(&cmd);
  return ok;
}

bool run_test_clear_all(char *test_name, char *base_cmd) {
  String_builder cmd = str_new();
  str_append(&cmd, base_cmd);
  str_append(&cmd, "\"clear all\"");

  int cmd_ret;
  bool ok = (run_test_execute(test_name, &cmd, &cmd_ret) && cmd_ret == 0);
  str_free(&cmd);
  if (!ok) return false;

  return ok;
}

bool run_test_compare_state(Test *test, const char *state_path, bool *same_state) {
  FILE *state_file = fopen(state_path, "r");
  if (!state_file) {
    printf("Unable to open the state file (%s)!\n", state_path);
    return false;
  }

  String_builder exp_state_path = str_new();
  if (test->expect_state_unchanged) {
    str_append(&exp_state_path, state.initial_states_path);
    str_append_to_path(&exp_state_path, test->state);
    str_append(&exp_state_path, ".idea");
  } else {
    str_append(&exp_state_path, state.final_states_path);
    str_append_to_path(&exp_state_path, test->name);
    str_append(&exp_state_path, ".idea");
  }
  FILE *exp_state_file = fopen(str_to_cstr(exp_state_path), "r");
  if (!exp_state_file) {
    fclose(state_file);
    printf("Unable to open the expected state file (%s)!\n", str_to_cstr(exp_state_path));
    str_free(&exp_state_path);
    return false;
  }
  str_free(&exp_state_path);

  String_builder line_state = str_new(), line_exp_state = str_new();
  bool read_state = str_read_line(state_file, &line_state),
       read_exp_state = str_read_line(exp_state_file, &line_exp_state);

  while (read_state && read_exp_state) {
    bool comparable_line = true;
    for (unsigned int i=0; i<incomparable_lines_count; i++) {
      if (cstr_starts_with(str_to_cstr(line_state), incomparable_lines[i])) {
        comparable_line = false;
        break;
      }
    }

    if (comparable_line && !str_equals(line_state, line_exp_state)) {
      printf("State differs: \n    - Actual:   %s\n    - Expected: %s\n", str_to_cstr(line_state), str_to_cstr(line_exp_state));
      break;
    }

    str_clean(&line_state);
    str_clean(&line_exp_state);

    read_state = str_read_line(state_file, &line_state);
    read_exp_state = str_read_line(exp_state_file, &line_exp_state);
  }

  fclose(state_file);
  fclose(exp_state_file);

  str_free(&line_state);
  str_free(&line_exp_state);

  *same_state = !(read_state || read_exp_state);
  return true;
}

void run_test(Test *test) {
  String_builder cmd_test = str_new();
  char *base_cmd = run_test_generate_base_command();

  if (test->state) {
    test->results.state_applied = (run_test_import_state(base_cmd, test))
                                  ? RESULT_PASSED
                                  : RESULT_FAILED;

    if (test->results.state_applied == RESULT_FAILED) {
      printf("Error importing the state\n");
      goto exit;
    }
  } else {
    test->results.state_applied = RESULT_NOT_SPECIFIED;
  }

  if (!list_is_empty(test->instructions)) {
    if (!run_test_generate_command(base_cmd, test, &cmd_test)) {
      printf("No instruction was specified\n");
      goto exit;
    }

    int cmd_test_ret;
    if (!run_test_execute(test->name, &cmd_test, &cmd_test_ret)) {
      printf("Error executing the instructions\n");
      goto exit;
    }

    test->results.expected_return = (cmd_test_ret == test->expected_return)
                                    ? RESULT_PASSED
                                    : RESULT_FAILED;
  } else {
    test->results.expected_return = RESULT_NOT_SPECIFIED;
  }

  if (!run_test_export_state(test->name, base_cmd)) {
    printf("Unable to export the state\n");
    goto exit;
  }

  bool same_state;
  if (!run_test_compare_state(test, state.idea_export_path, &same_state)) goto exit;
  test->results.expected_state = (same_state) ? RESULT_PASSED : RESULT_FAILED;

exit:
  if (run_test_clear_all(test->name, base_cmd)) {
    test->results.clear_after_test = RESULT_PASSED;
  } else {
    printf("Unable to clear all the ToDos");
    test->results.clear_after_test = RESULT_FAILED;
  }

  free(base_cmd);
  str_free(&cmd_test);
}

bool initialize_and_create_log_dir() {
  String_builder path = str_new();
  str_append(&path, state.repo_path);
  str_append_to_path(&path, "tests/logs");
  if (!create_dir_if_not_exists(str_to_cstr(path))) return false;

  time_t t = time(NULL);
  struct tm *tm = localtime(&t);

  str_append_to_path(&path, "");
  str_append_uint(&path, tm->tm_year+1900);
  str_append(&path, "-");
  str_append_uint(&path, tm->tm_mon+1);
  str_append(&path, "-");
  str_append_uint(&path, tm->tm_mday);
  str_append(&path, " ");
  str_append_uint(&path, tm->tm_hour);
  str_append(&path, ":");
  str_append_uint(&path, tm->tm_min);
  str_append(&path, ":");
  str_append_uint(&path, tm->tm_sec);

  state.logs_path = str_to_cstr(path);
  return create_dir_if_not_exists(str_to_cstr(path));
}

bool initialize_paths() {
  if (!state.repo_path) state.repo_path = "./";

  String_builder path = str_new();
  str_append(&path, state.repo_path);
  str_append_to_path(&path, "tests/tests");
  state.tests_filepath = strdup(str_to_cstr(path));
  str_clean(&path);

  str_append(&path, state.repo_path);
  str_append_to_path(&path, "tests/states/initial");
  state.initial_states_path = strdup(str_to_cstr(path));
  str_clean(&path);

  str_append(&path, state.repo_path);
  str_append_to_path(&path, "tests/states/final");
  state.final_states_path = strdup(str_to_cstr(path));
  str_clean(&path);

  if (!str_append_from_shell_variable(&path, "TMPDIR")) return false;
  str_append_to_path(&path, "idea_tests");
  state.idea_config_path = strdup(str_to_cstr(path));
  str_clean(&path);

  if (!str_append_from_shell_variable(&path, "TMPDIR")) return false;
  str_append_to_path(&path, "idea_tests_export.idea");
  state.idea_export_path = strdup(str_to_cstr(path));
  str_clean(&path);

  str_free(&path);
  if (state.log && !initialize_and_create_log_dir()) return false;
  return true;
}

void free_state() {
  if (state.tests_filepath) free(state.tests_filepath);
  if (state.initial_states_path) free(state.initial_states_path);
  if (state.final_states_path) free(state.final_states_path);
  if (state.logs_path) free(state.logs_path);
  if (state.idea_config_path) free(state.idea_config_path);
  if (state.idea_export_path) free(state.idea_export_path);
}

bool parse_args(int argc, char *argv[], int *ret) {
  for (int i=1; i < argc; i++) {
    if (!strcmp(argv[i], "-l")) {
      state.log = true;

    } else if (!strcmp(argv[i], "-h")) {
      printf("Commands:\n");
      printf("\t-h\tPrint help\n");
      printf("\t-l\tEnable logs: dump al the commands and it's output\n");
      printf("\t-p\tSpecify the path to the idea repository directory\n");
      return false;

    } else if (!strcmp(argv[i], "-p")) {
      if (i == argc-1) {
        printf("The repo path is not provided!\n");
        *ret = 1;
        return false;
      }

      if (state.repo_path) {
        printf("Error in the %dº argument: Repo path already provided!\n", i);
        *ret = 1;
        return false;
      }

      state.repo_path = argv[++i];

    } else {
      printf("Unrecognized arg (%dº): %s\n", i, argv[i]);

      *ret = 1;
      return false;
    }
  }

  return true;
}

int main(int argc, char *argv[]) {
  List tests = list_new();

  int ret = 0;
  if (!parse_args(argc, argv, &ret)) goto exit;

  if (!initialize_paths()) {
    ret = 1;
    goto exit;
  }

  printf("Config:\n");
  printf("- Repo path: %s\n", state.repo_path);
  printf("- Logs: %s\n", (state.log) ? state.logs_path : "Disabled");
  printf("\n");

  if (!get_tests(&tests)) {
    ret = 1;
    goto exit;
  }

  const unsigned int test_name_length = 40;
  print_results_header(test_name_length);

  List_iterator iterator = list_iterator_create(tests);
  while (list_iterator_next(&iterator)) {
    Test *test = list_iterator_element(iterator);
    run_test(test);
    print_results(*test, test_name_length);
  }

exit:
  list_destroy(&tests, (void (*)(void *))free_test);
  free_state();
  return ret;
}
