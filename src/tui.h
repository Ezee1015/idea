#ifndef TUI_H
#define TUI_H

#include <ncurses.h>

typedef struct {
  unsigned int width;
  unsigned int height;
} Size;

typedef struct {
  unsigned int x;
  unsigned int y;
} Point;

void message(char *title, char *msg);

void draw_window(void);
int window_app(void);

#endif // TUI_H
