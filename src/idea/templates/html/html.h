#ifndef HTML_H
#define HTML_H

#include "../../../utils/list.h"
#include "../../../../build/src/html_resources.h"

#define TASKS_LEVEL_INDENTATION_PX 30

bool generate_html(FILE *f);

// Global variables for the HTML
extern List html_todo_list;

#undef INITIALIZE_GLOBAL_VARS
#define INITIALIZE_GLOBAL_VARS() \
    List html_todo_list = list_new();

#include "../template_idea.h"

#endif // HTML_H
