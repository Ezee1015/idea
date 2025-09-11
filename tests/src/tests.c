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
  .valgrind = false,
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

char *result_to_cstr(Case r) {
  switch (r.result) {
    case RESULT_NOT_TESTED:
      if (r.memory_leak) abort();
      return ANSI_YELLOW CASE_UNTESTED ANSI_RESET;

    case RESULT_NOT_SPECIFIED:
      if (r.memory_leak) abort();
      return ANSI_GRAY CASE_NOT_SPECIFIED ANSI_RESET;

    case RESULT_PASSED:
      return (r.memory_leak)
             ? ANSI_YELLOW CASE_LEAK   ANSI_RESET
             : ANSI_GREEN  CASE_PASSED ANSI_RESET;

    case RESULT_FAILED:
      return (r.memory_leak)
             ? ANSI_RED CASE_LEAK   ANSI_RESET
             : ANSI_RED CASE_FAILED ANSI_RESET;
  }
  return NULL;
}

void print_results_header(FILE *output, unsigned int test_length) {
  char *cases_name[] = {
#define X(s) #s,
  CASES()
#undef X
  };

  // From https://github.com/bext-lang/b/blob/c6a21ba4c87ff1c304543c74dd910de34edad445/src/btest.rs#L228
  for (unsigned int i = 0; i < sizeof(cases_name)/sizeof(char *); i++) {
      char *test_name = cases_name[i];
      fprintf(output, "%*s", test_length + CASE_LENGTH / 2 + 1, "");
      for (unsigned int x = 0; x < i; x++) fprintf(output, "│%*s", CASE_LENGTH, "");
      fprintf(output, "┌─ ");

      // Pretty print the test case name
      fputc(*test_name - 32, output); // capitalize the first letter
      while ( *(++test_name) != '\0' ) {
        if (*test_name == '_') fputc(' ', output);
        else fputc(*test_name, output);
      }
      fputc('\n', output);
  }
}

void print_results(FILE *output, Test test, unsigned int test_name_length) {
  fprintf(output, "%*s", test_name_length, test.name);

#define X(s) fprintf(output, " %s", result_to_cstr(test.results.s));
  CASES()
#undef X

  fprintf(output, "\n");
}

char *run_test_generate_base_command(bool valgrind) {
  String_builder base_cmd = str_new();
  str_append(&base_cmd, "IDEA_CONFIG_PATH=\"");
  str_append(&base_cmd, state.idea_config_path);
  str_append(&base_cmd, "\" ");
  if (valgrind) str_append(&base_cmd, VALGRIND_CMD " ");
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
      // printf("Unable to open log: %s!\n", str_to_cstr(log_path));
      str_free(&log_path);
      return false;
    }
    fprintf(log, "-----> Running: %s\n", str_to_cstr(*cmd));
    fclose(log);

    str_append(cmd, " >> \"");
    str_append_str(cmd, log_path);
    str_append(cmd, "\" 2>&1");
    str_free(&log_path);
  } else {
    str_append(cmd, " > /dev/null 2>&1");
  }

  int system_ret = system(str_to_cstr(*cmd));
  if (system_ret == -1 || !WIFEXITED(system_ret)) return false;
  *ret = WEXITSTATUS(system_ret);
  return true;
}

bool run_test_case_import_initial_state(Test *t, List *messages, char *base_cmd, bool valgrind) {
  String_builder cmd = str_new();
  str_append(&cmd, base_cmd);
  str_append(&cmd, "\"import_no_diff ");
  str_append(&cmd, state.initial_states_path);
  str_append_to_path(&cmd, t->state);
  str_append(&cmd, ".idea\"");

  int cmd_ret;
  bool ok = run_test_execute(t->name, &cmd, &cmd_ret);
  str_free(&cmd);
  if (!ok) {
    APPEND_TO_MESSAGES(t, "An error occurred while importing the state");
    return false;
  }

  if (valgrind) t->results.import_initial_state.memory_leak = (cmd_ret == VALGRIND_LEAK_EXIT_CODE);
  else t->results.import_initial_state.result = (cmd_ret == 0) ? RESULT_PASSED : RESULT_FAILED;

  if (t->results.import_initial_state.result != RESULT_PASSED) {
    APPEND_TO_MESSAGES(t, "Importing the initial state failed");
    return false;
  }

  return true;
}

bool run_test_case_expected_return(Test *t, List *messages, char *base_cmd, bool valgrind) {
  if (list_is_empty(t->instructions)) {
      t->results.expected_return.result = RESULT_NOT_SPECIFIED;
      return true;
  }

  String_builder cmd = str_new();
  List_iterator iterator = list_iterator_create(t->instructions);
  str_append(&cmd, base_cmd);
  while (list_iterator_next(&iterator)) {
    char *instruction = list_iterator_element(iterator);

    str_append(&cmd, "\"");
    str_append(&cmd, instruction);
    str_append(&cmd, "\" ");
  }

  int cmd_ret;
  bool ok = run_test_execute(t->name, &cmd, &cmd_ret);
  str_free(&cmd);
  if (!ok) {
    APPEND_TO_MESSAGES(t, "An error occurred while executing the commands");
    return false;
  }

  if (valgrind) t->results.expected_return.memory_leak = (cmd_ret == VALGRIND_LEAK_EXIT_CODE);
  else t->results.expected_return.result = (cmd_ret == t->expected_return) ? RESULT_PASSED : RESULT_FAILED;

  if (t->results.expected_return.result != RESULT_PASSED) {
    APPEND_TO_MESSAGES(t, "Executing the commands failed");
    return false;
  }

  return true;
}

bool run_test_case_export_final_state(Test *t, List *messages, char *base_cmd, bool valgrind) {
  String_builder cmd = str_new();
  str_append(&cmd, base_cmd);
  str_append(&cmd, "\"export ");
  str_append(&cmd, state.idea_export_path);
  str_append(&cmd, "\" ");

  int cmd_ret;
  bool ok = run_test_execute(t->name, &cmd, &cmd_ret);
  str_free(&cmd);
  if (!ok) {
    APPEND_TO_MESSAGES(t, "An error occurred while exporting the state");
    return false;
  }

  if (valgrind) t->results.export_final_state.memory_leak = (cmd_ret == VALGRIND_LEAK_EXIT_CODE);
  else t->results.export_final_state.result = (cmd_ret == 0) ? RESULT_PASSED : RESULT_FAILED;

  if (t->results.export_final_state.result != RESULT_PASSED) {
    APPEND_TO_MESSAGES(t, "Exporting the state failed");
    return false;
  }

  return true;
}

bool run_test_case_expected_final_state(Test *t, List *messages, char *base_cmd, bool valgrind) {
  (void) base_cmd;

  if (valgrind) return true;

  FILE *state_file = fopen(state.idea_export_path, "r");
  if (!state_file) {
    String_builder sb = str_new();
    str_append(&sb, "Unable to open the state file (");
    str_append(&sb, state.idea_export_path);
    str_append(&sb, ")!\n");
    APPEND_TO_MESSAGES(t, str_to_cstr(sb));
    str_free(&sb);

    return false;
  }

  String_builder exp_state_path = str_new();
  if (t->expect_state_unchanged) {
    str_append(&exp_state_path, state.initial_states_path);
    str_append_to_path(&exp_state_path, t->state);
  } else {
    str_append(&exp_state_path, state.final_states_path);
    str_append_to_path(&exp_state_path, t->name);
  }
  str_append(&exp_state_path, ".idea");
  FILE *exp_state_file = fopen(str_to_cstr(exp_state_path), "r");
  if (!exp_state_file) {
    fclose(state_file);

    String_builder sb = str_new();
    str_append(&sb, "Unable to open the expected state file (");
    str_append(&sb, str_to_cstr(exp_state_path));
    str_append(&sb, ")!\n");
    APPEND_TO_MESSAGES(t, str_to_cstr(sb));
    str_free(&sb);

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
      String_builder sb = str_new();
      str_append(&sb, "State differs:\n\t- Actual:   ");
      str_append_str(&sb, line_state);
      str_append(&sb, "\n\t- Expected: ");
      str_append_str(&sb, line_exp_state);
      APPEND_TO_MESSAGES(t, str_to_cstr(sb));
      str_free(&sb);

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

  bool same_state = !(read_state || read_exp_state);
  t->results.expected_final_state.result = (same_state) ? RESULT_PASSED : RESULT_FAILED;

  return true;
}

bool run_test_case_clear_after_test(Test *t, List *messages, char *base_cmd, bool valgrind) {
  String_builder cmd = str_new();
  str_append(&cmd, base_cmd);
  str_append(&cmd, "\"clear all\"");

  int cmd_ret;
  bool ok = run_test_execute(t->name, &cmd, &cmd_ret);
  str_free(&cmd);
  if (!ok) {
    APPEND_TO_MESSAGES(t, "An error occurred while clearing the ToDos");
    return false;
  }

  if (valgrind) t->results.clear_after_test.memory_leak = (cmd_ret == VALGRIND_LEAK_EXIT_CODE);
  else t->results.clear_after_test.result = (cmd_ret == 0) ? RESULT_PASSED : RESULT_FAILED;

  if (t->results.clear_after_test.result != RESULT_PASSED) {
    APPEND_TO_MESSAGES(t, "Unable to clear all the ToDos");
    return false;
  }

  return true;
}

void run_test(Test *test, List *messages, bool valgrind) {
  if (valgrind) {
    String_builder log_path = str_new();
    str_append(&log_path, state.logs_path);
    str_append_to_path(&log_path, test->name);
    str_append(&log_path, ".txt");
    FILE *log = fopen(str_to_cstr(log_path), "a");
    str_free(&log_path);
    if (!log) {
      APPEND_TO_MESSAGES(test, "Unable to open the log file!");
      return;
    }

    fprintf(log, "\n========================== VALGRIND CHECK ==========================\n\n");
    fclose(log);
  }

  char *base_cmd = run_test_generate_base_command(valgrind);

  if (valgrind) {
    #define should_test(s) (test->results.s.result != RESULT_NOT_SPECIFIED && test->results.s.result != RESULT_NOT_TESTED)
    #define X(s) if (should_test(s)) run_test_case_##s(test, messages, base_cmd, valgrind);

      CASES()

    #undef X
    #undef should_test
  } else {
    #define X(s) if (!run_test_case_##s(test, messages, base_cmd, valgrind)) goto exit;

      CASES()

    #undef X
  }

exit:
  // Try to clean the mess if something went wrong
  if (test->results.clear_after_test.result == RESULT_NOT_TESTED)
    run_test_case_clear_after_test(test, messages, base_cmd, false);

  free(base_cmd);
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

bool is_valgrind_available() {
  int ret = system("command -v valgrind >/dev/null 2>&1");
  return (ret != -1 && WIFEXITED(ret) && ret == 0);
}

bool parse_args(int argc, char *argv[], int *ret) {
  for (int i=1; i < argc; i++) {
    if (!strcmp(argv[i], "-l")) {
      state.log = true;

    } else if (!strcmp(argv[i], "-v")) {
      state.valgrind = true;
      if (!is_valgrind_available()) {
        printf("Unable to find valgrind\n");
        *ret = 1;
        return false;
      }

    } else if (!strcmp(argv[i], "-h")) {
      printf("Commands:\n");
      printf("\t-h\tPrint help\n");
      printf("\t-l\tEnable logs: dump al the commands and it's output\n");
      printf("\t-p\tSpecify the path to the idea repository directory\n");
      printf("\t-v\tRun checks with valgrind too\n");
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
  List messages = list_new();

  int ret = 0;
  if (!parse_args(argc, argv, &ret)) goto exit;

  if (!initialize_paths()) {
    ret = 1;
    goto exit;
  }

  puts(ANSI_GRAY);
  printf("Config:\n");
  printf("- Repo path: %s\n", state.repo_path);
  printf("- Logs: %s\n", (state.log) ? state.logs_path : "Disabled");
  printf("- Valgrind check: %s\n", (state.valgrind) ? "Enabled" : "Disabled");
  puts(ANSI_RESET);
  printf("\n");

  if (!get_tests(&tests)) {
    ret = 1;
    goto exit;
  }

  const int padding = 5;
  unsigned int test_name_length = 0;
  List_iterator iterator = list_iterator_create(tests);
  while (list_iterator_next(&iterator)) {
    Test *test = list_iterator_element(iterator);
    const unsigned int length = strlen(test->name) + padding;
    if (length > test_name_length) test_name_length = length;
  }

  FILE *log = NULL;
  if (state.log) {
    String_builder log_path = str_new();
    str_append(&log_path, state.logs_path);
    str_append_to_path(&log_path, "final_results.txt");
    log = fopen(str_to_cstr(log_path), "w");
    str_free(&log_path);
    if (!log) {
      ret = 1;
      goto exit;
    }

    print_results_header(log, test_name_length);
  }

  print_results_header(stdout, test_name_length);

  iterator = list_iterator_create(tests);
  while (list_iterator_next(&iterator)) {
    Test *test = list_iterator_element(iterator);
    run_test(test, &messages, false);
    if (state.valgrind) run_test(test, &messages, true);
    print_results(stdout, *test, test_name_length);
    if (state.log) print_results(log, *test, test_name_length);
  }

  if (!list_is_empty(messages)) printf(ANSI_GRAY "\n\nInformation:\n\n" ANSI_RESET);
  iterator = list_iterator_create(messages);
  while (list_iterator_next(&iterator)) {
    char *message = list_iterator_element(iterator);
    printf("%s\n", message);
  }

exit:
  list_destroy(&tests, (void (*)(void *))free_test);
  list_destroy(&messages, (void (*)(void *))free);
  if (log) fclose(log);
  free_state();
  return ret;
}
