#ifndef TEMPLATE_H
#define TEMPLATE_H

#include <stdio.h>

#include "../utils/list.h"
#include "../todo_list.h"

// includes for the template.html
#include <time.h>

#define FPRINTF_ERROR() do {                                \
    APPEND_TO_BACKTRACE(BACKTRACE_ERROR, "fprintf failed"); \
    return false;                                           \
} while (0);

// Macros to use in the HTML Template (to simplify the c code there)
#define PRINT(fmt, ...) if (fprintf(f, fmt, __VA_ARGS__) < 0) FPRINTF_ERROR();
#define CHAR(c) if (fputc(c, f) == EOF) FPRINTF_ERROR();

#define CSTR(cstr) PRINT("%s", cstr)
#define INT(int_) PRINT("%d", int_)
#define UINT(uint) PRINT("%u", uint)
#define UNIX_TIME(time) PRINT("%ld", *time)
#define TIME(unix_time) do { \
    char b[32]; \
    strftime(b, 20, "%Y/%m/%d %H:%M:%S", localtime((time_t*) &unix_time));\
    CSTR(b) \
  } while(0);

#define ERROR(msg) do {                        \
    APPEND_TO_BACKTRACE(BACKTRACE_ERROR, msg); \
    return false;                              \
} while (0);
#define ERROR_FMT(msg, ...) do {                        \
    APPEND_TO_BACKTRACE(BACKTRACE_ERROR, msg, __VA_ARGS__); \
    return false;                              \
} while (0);

#define TASKS_LEVEL_INDENTATION_PX 30

bool generate_html(FILE *f, List todo_list);

#endif // TEMPLATE_H
