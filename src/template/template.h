#ifndef TEMPLATE_H
#define TEMPLATE_H

#include <stdio.h>
#include "../utils/list.h"
#include "../todo_list.h"

#define FPRINTF_ERROR() ACTION_RETURN(RETURN_ERROR, "fprintf failed")

// Macros to use in the HTML Template (to simplify the c code there)
#define CSTR(cstr) if (fprintf(f, "%s", cstr) < 0) return FPRINTF_ERROR();
#define CHAR(c) if (fputc(c, f) == EOF) return FPRINTF_ERROR();
#define UINT(uint) if (fprintf(f, "%u", uint) < 0) return FPRINTF_ERROR();
#define UNIX_TIME(time) if (fprintf(f, "%ld", *time) < 0) return FPRINTF_ERROR();
#define ERROR(msg) ACTION_RETURN(RETURN_ERROR, msg)

Action_return generate_html(FILE *f, List todo_list);

#endif // TEMPLATE_H
