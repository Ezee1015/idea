#include "cli.h"
#include "db.h"

void print_todo(void) {
  List_iterator iterator = list_iterator_create(todo_list);
  while (list_iterator_next(&iterator)) {
    Todo *todo = list_iterator_element(iterator);
    printf("%d) %s\n", list_iterator_index(iterator)+1, todo->data);
  }
}
