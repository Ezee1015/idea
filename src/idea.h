#ifndef IDEA_H
#define IDEA_H

#include "db.h"

#define CMD_EXIT "q"

typedef struct {
  unsigned int width;
  unsigned int height;
} Size;

typedef struct {
  unsigned int x;
  unsigned int y;
} Point;

#define INFO(msg) message("INFO", msg)

#define ERROR_AND_EXIT(msg) do { \
  message("ERROR", msg);         \
  endwin();                      \
  exit(1);                       \
} while(0)                       \

void message(char *title, char *msg);

void draw_window(void);
int window_app(void);
void print_todo(void);

int main(int argc, char *argv[]);

#endif
