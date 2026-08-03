#include "backtrace.h"
#include <stdlib.h>

void free_backtrace_item(Backtrace_item *b) {
  free(b->message);
  free(b);
}
