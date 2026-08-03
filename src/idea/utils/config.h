#ifndef CONFIG_H
#define CONFIG_H

#include <stdio.h>
#include <stdbool.h>

#define CONFIG_PATH ".config"
#define CONFIG_FILENAME "idea.conf"

typedef struct {
  char *hostname;
} Config;

bool load_config();
bool fill_config_with_defaults(Config *config);
bool write_config_file(FILE *file, Config config);
void free_config(Config c);

#endif // CONFIG_H
