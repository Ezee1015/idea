#ifndef TEMPLATE_H
#define TEMPLATE_H

#include <stdio.h>
#include <time.h>
#include "../utils/list.h"
#include "../todo_list.h"

#define CSTR(cstr) if (fprintf(f, "%s", cstr) < 0) return false;

bool generate_html(FILE *f, List todo_list);

#endif // TEMPLATE_H
