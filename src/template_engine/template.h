#ifndef TEMPLATE_H
#define TEMPLATE_H

#include <stdio.h>
#include <stdbool.h>
#include <time.h>

#ifndef INITIALIZE_GLOBAL_VARS
#define INITIALIZE_GLOBAL_VARS()
#endif // INITIALIZE_GLOBAL_VARS

#ifndef ERROR
#define ERROR(msg) do {\
    printf(msg);       \
    return false;      \
} while (0);
#endif // ERROR

#ifndef ERROR_FMT
#define ERROR_FMT(msg, ...) do { \
    printf(msg, __VA_ARGS__);    \
    return false;                \
} while (0);
#endif // ERROR_FMT

#define FPRINTF_ERROR() ERROR("fprintf failed");

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

#endif // TEMPLATE_H
