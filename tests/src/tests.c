#include <stdio.h>
#include <stdbool.h>
#include <malloc.h>
#include <dirent.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <sys/stat.h>

#include "../../src/cli.h"
#include "../../src/parser.h"
#include "../../src/todo_list.h"

#include "tests.h"

Tests_state state = {
  .idea_config_path = NULL,
  .idea_export_path = NULL,
  .idea_path = "build/idea",
  .idea_lock_filepath = NULL,

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

    } else if (!strcmp(tag, "should_fail")) {
      free(tag);

      if (!test) {
        printf("%s:%d: [ERROR] Name of the test is not provided!\n", state.tests_filepath, line_nr);
        ret = false;
        goto exit;
      }

      if (test->should_fail_execution) {
        printf("%s:%d: [ERROR] Should fail already provided!\n", state.tests_filepath, line_nr);
        ret = false;
        goto exit;
      }

      test->should_fail_execution = true;
      test->expect_state_unchanged = true;

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

    case RESULT_COUNT:
      abort();
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

void collect_statistics(Statistics *stats, Test *t) {
#define X(s) do { \
    stats->count++; \
    stats->results[t->results.s.result]++; \
    if (t->results.s.memory_leak) { \
      switch (t->results.s.result) { \
        case RESULT_FAILED: stats->bad_mem_leaks++;  break; \
        case RESULT_PASSED: stats->good_mem_leaks++; break; \
        default: abort(); \
      } \
    } \
  } while (0);

  CASES()

#undef X
}

void print_statistics(Statistics stats) {
  printf(ANSI_GRAY"\nStatistics: " ANSI_RESET "%d test cases\n", stats.count);

  for (int i=0; i<RESULT_COUNT; i++) {
    Case c = { .result = i };
    if (stats.results[i]) {
      printf("  - %s %d test cases ", result_to_cstr(c), stats.results[i]);
      switch ((Result_type) i) {
        case RESULT_NOT_TESTED:    printf("could not be tested\n"); break;
        case RESULT_FAILED:        printf("failed\n"); break;
        case RESULT_PASSED:        printf("passed\n"); break;
        case RESULT_NOT_SPECIFIED: printf("were not specified to be run\n"); break;
        case RESULT_COUNT:         abort();
      }
    }
  }

  putchar('\n');
  if (stats.good_mem_leaks)
    printf("  - " ANSI_YELLOW CASE_LEAK ANSI_RESET " %d test cases finished with memory leaks, but still passed the test case\n", stats.good_mem_leaks);

  if (stats.bad_mem_leaks)
    printf("  - " ANSI_RED CASE_LEAK ANSI_RESET " %d test cases finished with memory leaks and failed the test case\n", stats.bad_mem_leaks);
}

char *run_test_generate_base_command(bool valgrind) {
  String_builder base_cmd = str_create("IDEA_CONFIG_PATH=\"%s\" %s %s/%s",
                                       state.idea_config_path,
                                       (valgrind) ? VALGRIND_CMD : "",
                                       state.repo_path,
                                       state.idea_path);
  return str_to_cstr(base_cmd);
}

bool run_test_execute(Test *test, List *messages, String_builder *cmd, int *ret) {
  if (state.log) {
    String_builder log_path = str_create("%s/%s.txt", state.logs_path, test->name);
    FILE *log = fopen(str_to_cstr(log_path), "a");
    if (!log) {
      APPEND_WITH_FORMAT_TO_MESSAGES(test, "Unable to open log: %s", str_to_cstr(log_path));
      str_free(&log_path);
      return false;
    }
    fprintf(log, "-----> Running: %s\n", str_to_cstr(*cmd));
    fclose(log);

    str_append_with_format(cmd, " >> \"%s\" 2>&1", str_to_cstr(log_path));
    str_free(&log_path);
  } else {
    str_append(cmd, " > /dev/null 2>&1");
  }

  int system_ret = system(str_to_cstr(*cmd));

  if (access(state.idea_lock_filepath, F_OK) == 0) {
    if (remove(state.idea_lock_filepath) == 0) {
      APPEND_TO_MESSAGES(test, "The lock file was still present after the execution, it had to be removed!");
    } else {
      APPEND_TO_MESSAGES(test, "The lock file was still present after the execution and couldn't be removed!");
    }
  }

  if (system_ret == -1 || !WIFEXITED(system_ret)) return false;
  *ret = WEXITSTATUS(system_ret);
  return true;
}

bool run_test_case_import_initial_state(Test *t, List *messages, char *base_cmd, bool valgrind) {
  String_builder cmd = str_create("%s \"import_no_diff %s/%s.idea\"", base_cmd, state.initial_states_path, t->state);
  int cmd_ret;
  bool ok = run_test_execute(t, messages, &cmd, &cmd_ret);
  str_free(&cmd);
  if (!ok) return false;

  if (valgrind) t->results.import_initial_state.memory_leak = (cmd_ret == VALGRIND_LEAK_EXIT_CODE);
  else t->results.import_initial_state.result = (cmd_ret == 0) ? RESULT_PASSED : RESULT_FAILED;

  if (t->results.import_initial_state.result != RESULT_PASSED) {
    APPEND_TO_MESSAGES(t, "Importing the initial state failed");
    return false;
  }

  return true;
}

bool run_test_case_execution(Test *t, List *messages, char *base_cmd, bool valgrind) {
  if (list_is_empty(t->instructions)) {
      t->results.execution.result = RESULT_NOT_SPECIFIED;
      return true;
  }

  String_builder cmd = str_create("%s", base_cmd);
  List_iterator iterator = list_iterator_create(t->instructions);
  while (list_iterator_next(&iterator)) {
    char *instruction = list_iterator_element(iterator);
    str_append_with_format(&cmd, " \"%s\"", instruction);
  }

  int cmd_ret;
  bool ok = run_test_execute(t, messages, &cmd, &cmd_ret);
  str_free(&cmd);
  if (!ok) return false;

  if (valgrind) {
    t->results.execution.memory_leak = (cmd_ret == VALGRIND_LEAK_EXIT_CODE);
  } else {
    bool expected_return = (!t->should_fail_execution) ? (cmd_ret == 0) : (cmd_ret != 0);
    t->results.execution.result = (expected_return) ? RESULT_PASSED : RESULT_FAILED;
  }

  if (t->results.execution.result != RESULT_PASSED) {
    APPEND_TO_MESSAGES(t, "Executing the commands failed");
    return false;
  }

  return true;
}

bool run_test_case_export_final_state(Test *t, List *messages, char *base_cmd, bool valgrind) {
  String_builder cmd = str_create("%s \"export %s\"", base_cmd, state.idea_export_path);

  int cmd_ret;
  bool ok = run_test_execute(t, messages, &cmd, &cmd_ret);
  str_free(&cmd);
  if (!ok) return false;

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
    APPEND_WITH_FORMAT_TO_MESSAGES(t, "Unable to open the state file (%s)!", state.idea_export_path);
    return false;
  }

  String_builder exp_state_path = (t->expect_state_unchanged)
                                  ? str_create("%s/%s.idea", state.initial_states_path, t->state)
                                  : str_create("%s/%s.idea", state.final_states_path, t->name);
  FILE *exp_state_file = fopen(str_to_cstr(exp_state_path), "r");
  if (!exp_state_file) {
    fclose(state_file);
    APPEND_WITH_FORMAT_TO_MESSAGES(t, "Unable to open the expected state file (%s)!", str_to_cstr(exp_state_path));
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
      APPEND_WITH_FORMAT_TO_MESSAGES(t, "State differs:\n\t- Actual:   %s\n\t- Expected: %s", line_state, line_exp_state);
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

bool is_dir_empty(char *path) {
  DIR *dir = opendir(path);
  if (!dir) return false;

  int n = 0;
  while (n <= 2 && readdir(dir)) n++;

  closedir(dir);
  return (n <= 2);
}

bool is_config_clean(Test* t, List *messages) {
  // Check if the notes directory is empty
  String_builder sb = str_create("%s/notes", state.idea_config_path);
  bool notes_dir_empty = is_dir_empty(str_to_cstr(sb));
  str_free(&sb);
  if (!notes_dir_empty) {
    APPEND_TO_MESSAGES(t, "There's still notes inside the notes directory!");
    return false;
  }

  // Check if list of ToDos is empty
  sb = str_create("%s/" SAVE_FILENAME, state.idea_config_path);
  FILE *todos = fopen(str_to_cstr(sb), "r");
  str_free(&sb);
  if (!todos) {
    APPEND_TO_MESSAGES(t, "Unable to open the ToDo list to see if it's empty");
    return false;
  }
  unsigned int elements = list_peek_element_count_from_bfile(todos);
  fclose(todos);
  if (elements != 0) {
    APPEND_TO_MESSAGES(t, "There's still elements inside the ToDo list file");
    return false;
  }

  return true;
}

bool run_test_case_clear_after_test(Test *t, List *messages, char *base_cmd, bool valgrind) {
  String_builder cmd = str_create("%s \"clear all\"", base_cmd);

  int cmd_ret;
  bool ok = run_test_execute(t, messages, &cmd, &cmd_ret);
  str_free(&cmd);
  if (!ok) return false;

  if (!is_config_clean(t, messages)) {
    t->results.clear_after_test.result = RESULT_FAILED;
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
  if (state.log && valgrind) {
    String_builder log_path = str_create("%s/%s.txt", state.logs_path, test->name);
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
  time_t t = time(NULL);
  struct tm *tm = localtime(&t);

  String_builder path = str_create("%s/tests/logs", state.repo_path);
  if (!create_dir_if_not_exists(str_to_cstr(path))) return false;

  str_append_with_format(&path, "/%u-%u-%u %u:%u:%u",
                                tm->tm_year+1900,
                                tm->tm_mon+1,
                                tm->tm_mday,
                                tm->tm_hour,
                                tm->tm_min,
                                tm->tm_sec);

  state.logs_path = str_to_cstr(path);
  return create_dir_if_not_exists(str_to_cstr(path));
}

bool initialize_paths() {
  String_builder tmp = str_new();
  if (!str_append_from_shell_variable(&tmp, "TMPDIR")) return false;
  char *tmp_path = str_to_cstr(tmp);

  if (!state.repo_path) state.repo_path = ".";

  state.tests_filepath      = str_to_cstr(str_create("%s/tests/tests", state.repo_path));
  state.initial_states_path = str_to_cstr(str_create("%s/tests/states/initial", state.repo_path));
  state.final_states_path   = str_to_cstr(str_create("%s/tests/states/final", state.repo_path));
  state.idea_config_path    = str_to_cstr(str_create("%s/idea_tests", tmp_path));
  state.idea_export_path    = str_to_cstr(str_create("%s/idea_tests_export.idea", tmp_path));
  state.idea_lock_filepath  = str_to_cstr(str_create("%s/idea.lock", tmp_path));

  str_free(&tmp);
  if (state.log && !initialize_and_create_log_dir()) return false;
  return true;
}

void free_state() {
  if (state.tests_filepath) free(state.tests_filepath);
  if (state.initial_states_path) free(state.initial_states_path);
  if (state.final_states_path) free(state.final_states_path);
  if (state.logs_path) free(state.logs_path);
  if (state.idea_lock_filepath) free(state.idea_lock_filepath);
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
      if (state.repo_path[strlen(state.repo_path)-1] == '/') {
        printf("ERROR: Paths to directories should not end with '/'\n");
        *ret = 1;
        return false;
      }

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
  FILE *log = NULL;

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

  if (state.log) {
    String_builder log_path = str_create("%s/final_results.txt", state.logs_path);
    log = fopen(str_to_cstr(log_path), "w");
    str_free(&log_path);
    if (!log) {
      ret = 1;
      goto exit;
    }

    print_results_header(log, test_name_length);
  }

  print_results_header(stdout, test_name_length);

  Statistics stats = {0};

  iterator = list_iterator_create(tests);
  while (list_iterator_next(&iterator)) {
    Test *test = list_iterator_element(iterator);
    run_test(test, &messages, false);
    if (state.valgrind) run_test(test, &messages, true);
    print_results(stdout, *test, test_name_length);
    if (state.log) print_results(log, *test, test_name_length);
    collect_statistics(&stats, test);
  }

  print_statistics(stats);

  if (!list_is_empty(messages)) printf(ANSI_GRAY "\nInformation:\n\n" ANSI_RESET);
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
