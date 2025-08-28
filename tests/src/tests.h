#ifndef TESTS_H
#define TESTS_H

#include "../../src/utils/list.h"
#include "../../src/utils/string.h"

// This should be the same length
#define CASE_PASSED        "  "
#define CASE_FAILED        "  "
#define CASE_UNTESTED      "  "
#define CASE_NOT_SPECIFIED "  "
#define CASE_LENGTH 3 // Because it has some characters that break strlen()

typedef struct {
  char *idea_config_path;
  char *idea_export_path;
  char *idea_path;

  char *repo_path;

  char *tests_filepath;
  char *initial_states_path;
  char *final_states_path;
  char *logs_path;

  bool log;
} Tests_state;

typedef enum {
  RESULT_NOT_TESTED,
  RESULT_FAILED,
  RESULT_PASSED,
  RESULT_NOT_SPECIFIED,
} Result;

typedef struct {
  Result state_applied;
  Result clear_after_test;
  Result expected_return;
  Result expected_state;
} Test_result;

typedef struct {
  char *name;
  char *state;

  List instructions;

  int expected_return;
  bool expect_state_unchanged;

  Test_result results;
} Test;

void free_state();

void free_test(Test *test);

bool get_tests(List *tests);

char *result_to_cstr(Result r);
void print_results_header(unsigned int test_length);
void print_results(Test test, unsigned int test_name_length);

char *run_test_generate_base_command();
bool run_test_execute(char *test_name, String_builder *cmd, int *ret);
bool run_test_import_state(char *base_cmd, Test *test);
bool run_test_generate_command(char *base_cmd, Test *test, String_builder *cmd);
bool run_test_export_state(char *test_name, char *base_cmd);
bool run_test_clear_all(char *test_name, char *base_cmd);
bool run_test_compare_state(Test *test, const char *state_path, bool *same_state);
void run_test(Test *test);

bool create_dir_if_not_exists(char *dir_path);
bool initialize_paths();
bool initialize_and_create_log_dir();

bool parse_args(int argc, char *argv[], int *ret);
int main(int argc, char *argv[]);

#endif // TESTS_H
