#ifndef ZSH_COMPLETE_H
#define ZSH_COMPLETE_H

#include "../../../utils/list.h"
#include "../../cli.h"

#include <string.h>

bool generate_zsh_completion(FILE *f);

#define ZSH_COMMAND_LIST(functionality, functionality_count) do { \
    for (unsigned int i = 0; i < functionality_count; i++) {  \
        const Functionality func = functionality[i];          \
        PRINT("'%s:%s'", func.full_cmd, func.man.description); \
        CSTR("\n");                                           \
    }                                                         \
} while (0);

#define ZSH_COMMAND_SWITCH(functionality, functionality_count) do { \
    for (unsigned int _i = 0; _i < functionality_count; _i++) { \
        const Functionality func = functionality[_i]; \
        CSTR(func.full_cmd); if (func.abbreviation_cmd && func.abbreviation_cmd[0] != '\0') PRINT("|%s", func.abbreviation_cmd); CSTR(")\n"); \
            CSTR("PARAMS=(\n"); \
                for (int _x=0; func.man.parameters[_x]; _x++) { \
                    if (strcmp(func.man.parameters[_x], "")) PRINT("\"%s\"\n", func.man.parameters[_x]); \
                } \
            CSTR(")\n"); \
        CSTR(";;\n"); \
    } \
} while (0);

#include "../template_idea.h"

#endif // ZSH_COMPLETE_H
