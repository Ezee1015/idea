#ifndef TESTS_H
#define TESTS_H

#include "../../src/utils/list.h"
#include "../../src/utils/string.h"

// This should be the same length
#define CASE_PASSED        "  "
#define CASE_FAILED        "  "
#define CASE_UNTESTED      "  "
#define CASE_NOT_SPECIFIED "  "
#define CASE_LEAK          "  "
#define CASE_LENGTH 3 // Because it has some characters that break strlen()

#define MACRO_STR(n) #n
#define MACRO_INT_TO_STR(n) MACRO_STR(n)

#define VALGRIND_LEAK_EXIT_CODE 255 // Some random number
#define VALGRIND_CMD "valgrind --leak-check=full --show-leak-kinds=all --errors-for-leak-kinds=all --error-exitcode=" MACRO_INT_TO_STR(VALGRIND_LEAK_EXIT_CODE)

// X macro. References:
// - https://www.youtube.com/watch?v=PgDqBZFir1A
// - https://en.wikipedia.org/wiki/X_macro
#define CASES() \
  X(import_initial_state) \
  X(expected_return) \
  X(export_final_state) \
  X(expected_final_state) \
  X(clear_after_test)

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
  bool valgrind;
} Tests_state;

typedef enum {
  RESULT_NOT_TESTED,
  RESULT_FAILED,
  RESULT_PASSED,
  RESULT_NOT_SPECIFIED,
} Result_type;

typedef struct {
  Result_type result;
  bool memory_leak;
} Case;

typedef struct {
#define X(s) Case s;
  CASES()
#undef X
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

char *result_to_cstr(Case r);
void print_results_header(unsigned int test_length);
void print_results(Test test, unsigned int test_name_length);

#define X(s) bool run_test_case_##s(Test *t, char *base_cmd, bool valgrind);
  CASES()
#undef X

char *run_test_generate_base_command(bool valgrind);
bool run_test_execute(char *test_name, String_builder *cmd, int *ret);
void run_test(Test *test, bool valgrind);

bool create_dir_if_not_exists(char *dir_path);
bool initialize_paths();
bool initialize_and_create_log_dir();

bool is_valgrind_available();
bool parse_args(int argc, char *argv[], int *ret);
int main(int argc, char *argv[]);

#endif // TESTS_H
