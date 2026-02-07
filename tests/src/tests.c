#include <stdio.h>
#include <stdbool.h>
#include <malloc.h>
#include <dirent.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>

#include "../../src/parser.h"
#include "../../src/todo_list.h"

#include "tests.h"

Tests_state state = {
  .idea_path = "build/idea",

  .repo_path = NULL,
  .tmp_path = NULL,

  .tests_filepath = NULL,
  .initial_states_path = NULL,
  .final_states_path = NULL,
  .logs_path = NULL,

  .max_test_name_length = 0,

  .log = false,
  .valgrind = false,
  .threads = 1
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
  String_builder line = sb_new();
  unsigned int line_nr = 1;
  while (sb_read_line(tests_file, &line)) {
    if (!line.length) {
      if (test && !is_a_valid_test(test)) {
        ret = false;
        goto exit;
      }

      test = NULL;
      line_nr++;
      continue;
    }

    Input input_line = {
      .input = line.str,
      .cursor = 0,
      .length = line.length
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

      char *name = next_token(&input_line, 0);

      List_iterator iterator = list_iterator_create(*tests);
      while (list_iterator_next(&iterator)) {
        Test *t = list_iterator_element(iterator);
        if (!strcmp(t->name, name)) {
          printf("%s:%d: [ERROR] Already exists a test with the name of %s!\n", state.tests_filepath, line_nr, name);
          free(name);
          ret = false;
          goto exit;
        }
      }

      test = malloc(sizeof(Test));
      memset(test, 0 , sizeof(Test));
      test->name = name;
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

    sb_clean(&line);
    line_nr++;
  }

  if (test && !is_a_valid_test(test)) ret = false;

exit:
  sb_free(&line);
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

void print_results_header(FILE *output) {
  char *cases_name[] = {
#define X(s) #s,
  CASES()
#undef X
  };

  // From https://github.com/bext-lang/b/blob/c6a21ba4c87ff1c304543c74dd910de34edad445/src/btest.rs#L228
  for (unsigned int i = 0; i < sizeof(cases_name)/sizeof(char *); i++) {
      char *test_name = cases_name[i];
      fprintf(output, "%*s", state.max_test_name_length + CASE_LENGTH / 2 + 1, "");
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

void print_results(FILE *output, Test test) {
  fprintf(output, "%*s", state.max_test_name_length, test.name);

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

char *run_test_generate_base_command(Runner_data runner_data, bool valgrind) {
  String_builder base_cmd = sb_create("IDEA_LOCAL_PATH=\"%s\" %s %s/%s",
                                      runner_data.local_path,
                                      (valgrind) ? VALGRIND_CMD : "",
                                      state.repo_path,
                                      state.idea_path);
  return base_cmd.str;
}

bool run_test_execute(Runner_data *runner_data, Test *test, String_builder *cmd, int *ret) {
  if (state.log) {
    String_builder log_path = sb_create("%s/%s.txt", state.logs_path, test->name);
    FILE *log = fopen(log_path.str, "a");
    if (!log) {
      APPEND_WITH_FORMAT_TO_MESSAGES(runner_data, "Test", test->name, "Unable to open log: %s", log_path.str);
      sb_free(&log_path);
      return false;
    }
    fprintf(log, "-----> Running: %s\n", cmd->str);
    fclose(log);

    sb_append_with_format(cmd, " >> \"%s\" 2>&1", log_path.str);
    sb_free(&log_path);
  } else {
    sb_append(cmd, " > /dev/null 2>&1");
  }

  int system_ret = system(cmd->str);

  String_builder lock_filepath = sb_create("%s/" LOCK_FILENAME, runner_data->local_path);
  if (access(lock_filepath.str, F_OK) == 0) {
    if (remove(lock_filepath.str) == 0) {
      APPEND_TO_MESSAGES(runner_data, "Test", test->name, "The lock file was still present after the execution, it had to be removed!");
    } else {
      APPEND_TO_MESSAGES(runner_data, "Test", test->name, "The lock file was still present after the execution and couldn't be removed!");
    }
  }
  sb_free(&lock_filepath);

  if (system_ret == -1 || !WIFEXITED(system_ret)) return false;
  *ret = WEXITSTATUS(system_ret);
  return true;
}

bool run_test_case_import_initial_state(Runner_data *runner_data, Test *t, char *base_cmd, bool valgrind) {
  String_builder cmd = sb_create("%s \"import %s/%s.idea\"", base_cmd, state.initial_states_path, t->state);
  int cmd_ret;
  bool ok = run_test_execute(runner_data, t, &cmd, &cmd_ret);
  sb_free(&cmd);
  if (!ok) return false;

  if (valgrind) t->results.import_initial_state.memory_leak = (cmd_ret == VALGRIND_LEAK_EXIT_CODE);
  else t->results.import_initial_state.result = (cmd_ret == 0) ? RESULT_PASSED : RESULT_FAILED;

  if (t->results.import_initial_state.result != RESULT_PASSED) {
    APPEND_TO_MESSAGES(runner_data, "Test", t->name, "Importing the initial state failed");
    return false;
  }

  return true;
}

bool run_test_case_execution(Runner_data *runner_data, Test *t, char *base_cmd, bool valgrind) {
  if (list_is_empty(t->instructions)) {
      t->results.execution.result = RESULT_NOT_SPECIFIED;
      return true;
  }

  String_builder cmd = sb_create("%s", base_cmd);
  List_iterator iterator = list_iterator_create(t->instructions);
  while (list_iterator_next(&iterator)) {
    char *instruction = list_iterator_element(iterator);
    sb_append_with_format(&cmd, " \"%s\"", instruction);
  }

  int cmd_ret;
  bool ok = run_test_execute(runner_data, t, &cmd, &cmd_ret);
  sb_free(&cmd);
  if (!ok) return false;

  if (valgrind) {
    t->results.execution.memory_leak = (cmd_ret == VALGRIND_LEAK_EXIT_CODE);
  } else {
    bool expected_return = (!t->should_fail_execution) ? (cmd_ret == 0) : (cmd_ret != 0);
    t->results.execution.result = (expected_return) ? RESULT_PASSED : RESULT_FAILED;
  }

  if (t->results.execution.result != RESULT_PASSED) {
    APPEND_TO_MESSAGES(runner_data, "Test", t->name, "Executing the commands failed");
    return false;
  }

  return true;
}

bool run_test_case_export_final_state(Runner_data *runner_data, Test *t, char *base_cmd, bool valgrind) {
  String_builder cmd = sb_create("%s \"export %s\"", base_cmd, runner_data->export_filepath);

  int cmd_ret;
  bool ok = run_test_execute(runner_data, t, &cmd, &cmd_ret);
  sb_free(&cmd);
  if (!ok) return false;

  if (valgrind) t->results.export_final_state.memory_leak = (cmd_ret == VALGRIND_LEAK_EXIT_CODE);
  else t->results.export_final_state.result = (cmd_ret == 0) ? RESULT_PASSED : RESULT_FAILED;

  if (t->results.export_final_state.result != RESULT_PASSED) {
    APPEND_TO_MESSAGES(runner_data, "Test", t->name, "Exporting the state failed");
    return false;
  }

  return true;
}

bool run_test_case_expected_final_state(Runner_data *runner_data, Test *t, char *base_cmd, bool valgrind) {
  (void) base_cmd;

  if (valgrind) return true;

  FILE *state_file = fopen(runner_data->export_filepath, "r");
  if (!state_file) {
    APPEND_WITH_FORMAT_TO_MESSAGES(runner_data, "Test", t->name, "Unable to open the state file (%s)!", runner_data->export_filepath);
    return false;
  }

  String_builder exp_state_path = (t->expect_state_unchanged)
                                  ? sb_create("%s/%s.idea", state.initial_states_path, t->state)
                                  : sb_create("%s/%s.idea", state.final_states_path, t->name);
  FILE *exp_state_file = fopen(exp_state_path.str, "r");
  if (!exp_state_file) {
    fclose(state_file);
    APPEND_WITH_FORMAT_TO_MESSAGES(runner_data, "Test", t->name, "Unable to open the expected state file (%s)!", exp_state_path.str);
    sb_free(&exp_state_path);
    return false;
  }
  sb_free(&exp_state_path);

  String_builder line_state = sb_new(), line_exp_state = sb_new();
  bool read_state = sb_read_line(state_file, &line_state),
       read_exp_state = sb_read_line(exp_state_file, &line_exp_state);

  while (read_state && read_exp_state) {
    bool comparable_line = true;
    for (unsigned int i=0; i<incomparable_lines_count; i++) {
      if (cstr_starts_with(line_state.str, incomparable_lines[i])) {
        comparable_line = false;
        break;
      }
    }

    if (comparable_line && !sb_equals(line_state, line_exp_state)) {
      APPEND_WITH_FORMAT_TO_MESSAGES(runner_data, "Test", t->name, "State differs:\n\t- Actual:   %s\n\t- Expected: %s", line_state.str, line_exp_state.str);
      break;
    }

    sb_clean(&line_state);
    sb_clean(&line_exp_state);

    read_state = sb_read_line(state_file, &line_state);
    read_exp_state = sb_read_line(exp_state_file, &line_exp_state);
  }

  fclose(state_file);
  fclose(exp_state_file);

  sb_free(&line_state);
  sb_free(&line_exp_state);

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

bool clean_directory(char *path) {
  DIR *dir = opendir(path);
  if (!dir) return false;

  struct dirent *f = NULL;
  while ( (f = readdir(dir)) ) {
    if (!strcmp(f->d_name, "..") || !strcmp(f->d_name, ".")) continue;

    String_builder sb = sb_create("%s/%s", path, f->d_name);
    int ret = remove(sb.str);
    sb_free(&sb);
    if (ret) {
      closedir(dir);
      return false;
    }
  }

  closedir(dir);
  return true;
}

bool is_config_clean(Runner_data *runner_data, Test* t) {
  // Check for files that should have been removed after executing idea
  char *temp_files[] = {
    sb_create("%s/" NOTES_TEMP_FILENAME, runner_data->local_path).str,
    sb_create("%s/" LOCK_FILENAME, runner_data->local_path).str,
  };

  for (unsigned int i=0; i<sizeof(temp_files)/sizeof(char*); i++) {
    FILE *file = fopen(temp_files[i], "r");
    if (file) {
      fclose(file);

      String_builder tid = sb_create("%ld", pthread_self());
      APPEND_WITH_FORMAT_TO_MESSAGES(runner_data, "Thread", tid.str, "File %s is still present after execution! Removing it...\n", temp_files[i]);
      if (remove(temp_files[i]) == -1) {
        APPEND_WITH_FORMAT_TO_MESSAGES(runner_data, "Thread", tid.str, "Unable to remove the file %s.\n", temp_files[i]);
      }
      sb_free(&tid);
    }

    free(temp_files[i]);
  }

  // Check if list of ToDos is empty
  String_builder sb = sb_create("%s/" SAVE_FILENAME, runner_data->local_path);
  FILE *todos = fopen(sb.str, "r");
  sb_free(&sb);
  if (!todos) {
    APPEND_TO_MESSAGES(runner_data, "Test", t->name, "Unable to open the ToDo list to see if it's empty");
    return false;
  }
  unsigned int elements = list_peek_element_count_from_bfile(todos);
  fclose(todos);
  if (elements != 0) {
    APPEND_TO_MESSAGES(runner_data, "Test", t->name, "There's still elements inside the ToDo list file");
    return false;
  }

  return true;
}

bool run_test_case_clear_after_test(Runner_data *runner_data, Test *t, char *base_cmd, bool valgrind) {
  String_builder cmd = sb_create("%s \"clear all\"", base_cmd);

  int cmd_ret;
  bool ok = run_test_execute(runner_data, t, &cmd, &cmd_ret);
  sb_free(&cmd);
  if (!ok) return false;

  if (!is_config_clean(runner_data, t)) {
    t->results.clear_after_test.result = RESULT_FAILED;
    return false;
  }

  if (valgrind) t->results.clear_after_test.memory_leak = (cmd_ret == VALGRIND_LEAK_EXIT_CODE);
  else t->results.clear_after_test.result = (cmd_ret == 0) ? RESULT_PASSED : RESULT_FAILED;

  if (t->results.clear_after_test.result != RESULT_PASSED) {
    APPEND_TO_MESSAGES(runner_data, "Test", t->name, "Unable to clear all the ToDos");
    return false;
  }

  return true;
}

void run_test(Runner_data *runner_data, Test *test, bool valgrind) {
  if (state.log && valgrind) {
    String_builder log_path = sb_create("%s/%s.txt", state.logs_path, test->name);
    FILE *log = fopen(log_path.str, "a");
    sb_free(&log_path);
    if (!log) {
      APPEND_TO_MESSAGES(runner_data, "Test", test->name, "Unable to open the log file!");
      return;
    }

    fprintf(log, "\n========================== VALGRIND CHECK ==========================\n\n");
    fclose(log);
  }

  char *base_cmd = run_test_generate_base_command(*runner_data, valgrind);

  if (valgrind) {
    #define should_test(s) (test->results.s.result != RESULT_NOT_SPECIFIED && test->results.s.result != RESULT_NOT_TESTED)
    #define X(s) if (should_test(s)) run_test_case_##s(runner_data, test, base_cmd, valgrind);

      CASES()

    #undef X
    #undef should_test
  } else {
    #define X(s) if (!run_test_case_##s(runner_data, test, base_cmd, valgrind)) goto exit;

      CASES()

    #undef X
  }

exit:
  // Try to clean the mess if something went wrong
  if (test->results.clear_after_test.result == RESULT_NOT_TESTED)
    run_test_case_clear_after_test(runner_data, test, base_cmd, false);

  remove(runner_data->export_filepath); // Try to remove it
  free(base_cmd);
}

bool initialize_and_create_log_dir() {
  time_t t = time(NULL);
  struct tm *tm = localtime(&t);

  String_builder path = sb_create("%s/tests/logs", state.repo_path);
  if (!create_dir_if_not_exists(path.str)) return false;

  sb_append_with_format(&path, "/%u-%02u-%02u %02u:%02u:%02u",
                                tm->tm_year+1900,
                                tm->tm_mon+1,
                                tm->tm_mday,
                                tm->tm_hour,
                                tm->tm_min,
                                tm->tm_sec);

  state.logs_path = path.str;
  return create_dir_if_not_exists(path.str);
}

bool initialize_paths() {
  String_builder tmp = sb_new();
  if (!sb_append_from_shell_variable(&tmp, "TMPDIR")) return false;

  if (!state.repo_path) state.repo_path = ".";

  state.tmp_path            = tmp.str;
  state.tests_filepath      = sb_create("%s/tests/tests", state.repo_path).str;
  state.initial_states_path = sb_create("%s/tests/states/initial", state.repo_path).str;
  state.final_states_path   = sb_create("%s/tests/states/final", state.repo_path).str;

  if (state.log && !initialize_and_create_log_dir()) return false;
  return true;
}

void free_state() {
  if (state.tmp_path) free(state.tmp_path);
  if (state.tests_filepath) free(state.tests_filepath);
  if (state.initial_states_path) free(state.initial_states_path);
  if (state.final_states_path) free(state.final_states_path);
  if (state.logs_path) free(state.logs_path);
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
      printf("\t-j\tRun checks with the specified amount of threads\n");
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

    } else if (!strcmp(argv[i], "-j")) {
      if (i == argc-1) {
        printf("The amount of threads is not provided!\n");
        *ret = 1;
        return false;
      }

      i++;
      int t = atoi(argv[i]);
      if (t < 1) {
        printf("The amount of threads has to be a positive number!\n");
        *ret = 1;
        return false;
      }

      state.threads = t;

    } else {
      printf("Unrecognized arg (%dº): %s\n", i, argv[i]);

      *ret = 1;
      return false;
    }
  }

  return true;
}

void *test_runner(void *t_data) {
  Runner_data *data = (Runner_data *) t_data;

  data->local_path = sb_create("%s/%ld-idea", state.tmp_path, pthread_self()).str;
  if (!create_dir_if_not_exists(data->local_path)) abort();
  data->export_filepath = sb_create("%s/%ld-export", state.tmp_path, pthread_self()).str;

  for (unsigned int i=data->tests_range.start; i<=data->tests_range.end; i++) {
    Test *test = list_get(data->tests, i);
    run_test(data, test, false);
    if (state.valgrind) run_test(data, test, true);

    pthread_mutex_lock(data->m_stats);
      collect_statistics(data->stats, test);
    pthread_mutex_unlock(data->m_stats);

    pthread_mutex_lock(data->m_log);
      print_results(stdout, *test);
      if (state.log) print_results(data->log_file, *test);
    pthread_mutex_unlock(data->m_log);
  }

  // Remove config_path directory and it's associated files
  char *files_to_remove[] = {
    sb_create("%s/" SAVE_FILENAME, data->local_path).str,
    strdup(data->local_path),
  };

  bool ok = true;
  for (unsigned int i=0; i<sizeof(files_to_remove)/sizeof(char*); i++) {
    if (ok && remove(files_to_remove[i])) {
      String_builder tid = sb_create("%ld", pthread_self());
      APPEND_WITH_FORMAT_TO_MESSAGES(data, "Thread", tid.str, "Unable to remove %s. Reason: %s\n", files_to_remove[i], strerror(errno));
      sb_free(&tid);
      ok = false;
    }
    free(files_to_remove[i]);
  }

  free(data->local_path);
  free(data->export_filepath);
  return NULL;
}

void print_config() {
  printf(ANSI_GRAY "Config:\n");
  printf("- Repo path: %s\n", state.repo_path);
  printf("- Logs: %s\n", (state.log) ? state.logs_path : "Disabled");
  printf("- Valgrind check: %s\n", (state.valgrind) ? "Enabled" : "Disabled");
  printf("- Threads count: %u\n", state.threads);
  printf(ANSI_RESET "\n");
}

void find_max_test_name_length(List tests) {
  const int padding = 5;
  List_iterator iterator = list_iterator_create(tests);
  while (list_iterator_next(&iterator)) {
    Test *test = list_iterator_element(iterator);
    const unsigned int length = strlen(test->name) + padding;
    if (length > state.max_test_name_length) state.max_test_name_length = length;
  }
}

void print_messages(List messages) {
  if (!list_is_empty(messages)) printf(ANSI_GRAY "\nInformation:\n\n" ANSI_RESET);
  List_iterator iterator = list_iterator_create(messages);
  while (list_iterator_next(&iterator)) {
    char *message = list_iterator_element(iterator);
    printf("%s\n", message);
  }
}

int main(int argc, char *argv[]) {
  List tests = list_new();
  List messages = list_new();
  FILE *log = NULL;
  Runner_data *threads_data = NULL;
  pthread_t *thread_pool = NULL;
  pthread_mutex_t m_messages; pthread_mutex_init(&m_messages, NULL);
  pthread_mutex_t m_stats; pthread_mutex_init(&m_stats, NULL);
  pthread_mutex_t m_log; pthread_mutex_init(&m_log, NULL);

  int ret = 0;
  if (!parse_args(argc, argv, &ret)) goto exit;

  if (!initialize_paths()) {
    ret = 1;
    goto exit;
  }

  if (!get_tests(&tests)) {
    ret = 1;
    goto exit;
  }

  if (state.threads > list_size(tests)) state.threads = list_size(tests);

  print_config();

  find_max_test_name_length(tests);

  if (state.log) {
    String_builder log_path = sb_create("%s/final_results.txt", state.logs_path);
    log = fopen(log_path.str, "w");
    sb_free(&log_path);
    if (!log) {
      ret = 1;
      goto exit;
    }

    print_results_header(log);
  }
  print_results_header(stdout);

  thread_pool = malloc(sizeof(pthread_t) * state.threads);
  Statistics stats = {0};

  const unsigned int basic_tests_count_per_thread = list_size(tests) / state.threads;
  unsigned int additional_tests_count = list_size(tests) % state.threads;

  threads_data = malloc(sizeof(Runner_data) * state.threads);
  for (unsigned int i=0; i<state.threads; i++) {
    threads_data[i] = (Runner_data) {
      .messages = &messages,
      .tests = tests,
      .stats = &stats,
      .log_file = log,
      .m_messages = &m_messages,
      .m_stats = &m_stats,
      .m_log = &m_log,
    };

    threads_data[i].tests_range.start = (i == 0) ? 0 : threads_data[i-1].tests_range.end+1;
    threads_data[i].tests_range.end = threads_data[i].tests_range.start + basic_tests_count_per_thread - 1; // "- 1" because the range is inclusive
    if (additional_tests_count) {
      threads_data[i].tests_range.end++;
      additional_tests_count--;
    }
  }

  for (unsigned int i=0; i<state.threads; i++)
    pthread_create(thread_pool+i, NULL, test_runner, threads_data+i);

  for (unsigned int i=0; i<state.threads; i++)
    pthread_join(thread_pool[i], NULL);

  print_statistics(stats);

  print_messages(messages);

exit:
  list_destroy(&tests, (void (*)(void *))free_test);
  list_destroy(&messages, (void (*)(void *))free);
  pthread_mutex_destroy(&m_messages);
  pthread_mutex_destroy(&m_stats);
  pthread_mutex_destroy(&m_log);
  if (log) fclose(log);
  if (thread_pool) free(thread_pool);
  if (threads_data) free(threads_data);
  free_state();
  return ret;
}
