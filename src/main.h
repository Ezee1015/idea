#ifndef MAIN_H
#define MAIN_H

#include <stdbool.h>

typedef struct {
  char *backup_filepath;
  char *config_path;
  char *notes_path;
  char *tmp_path;
  char *lock_filepath;
} State;

extern State idea_state;

bool load_paths();
bool lock_file();
int main(int argc,char * argv[]);
bool parse_commands_cli(char * commands[],int count);
bool unlock_file();

#endif // MAIN_H
