#ifndef TEMPLATE_IDEA_H
#define TEMPLATE_IDEA_H

#include "../todo_list.h"

#define ERROR(msg) do {                        \
    APPEND_TO_BACKTRACE(BACKTRACE_ERROR, msg); \
    return false;                              \
} while (0);

#define ERROR_FMT(msg, ...) do {                        \
    APPEND_TO_BACKTRACE(BACKTRACE_ERROR, msg, __VA_ARGS__); \
    return false;                              \
} while (0);

#include "../../template_engine/template.h"

#endif // TEMPLATE_IDEA_H
