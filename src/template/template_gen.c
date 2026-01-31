#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>

#include "../utils/string.h"

// Inspired from: <https://github.com/rexim/tore/blob/main/src_build/tt.c>

#define DEFER(value) do { \
    ret = value;          \
    goto exit;            \
  } while(0)

#define C_KEY_OPEN "<c>"
#define C_KEY_CLOSE "</c>"

#define COMMENT(line) if (fprintf(template_c, "// " line "\n") < 0) return false
#define C_CODE(line)  if (fprintf(template_c, "%s // " __FILE__ ":%d\n", line, __LINE__) < 0) return false
#define EMPTY_LINE()  if (fprintf(template_c, "// " __FILE__ ":%d\n", __LINE__) < 0) return false

bool load_template(FILE *html_file, char **html_arr, long *html_arr_size) {
  if (!html_file) abort();
  if (!html_arr) abort();
  if (*html_arr) abort();

  if (fseek(html_file, 0, SEEK_END) == -1) return false;
  if ( (*html_arr_size = ftell(html_file)) == -1) return false;
  if (fseek(html_file, 0, SEEK_SET) == -1) return false;

  *html_arr = malloc(*html_arr_size+1);
  if ( !(*html_arr) ) return false;
  if (fread(*html_arr, *html_arr_size, 1, html_file) == 0) return false;
  (*html_arr)[*html_arr_size] = '\0';

  return true;
}

bool print_html_to_c_file(const char *html_filepath, long line, char *html_start, long html_length, FILE *template_c) {
  bool new_line = true;
  for (int i=0; i<html_length; i++) {
    if (new_line) {
      if (fprintf(template_c, "  if (fprintf(f, \"") < 0) return false;
      new_line = false;
    }

    const char c = html_start[i];
    switch (c) {
      case '\n':
        if (fprintf(template_c, "\\n") < 0) return false;
        new_line = true;
        line++;
        break;

      case '"':
        if (fprintf(template_c, "\\\"") < 0) return false;
        break;

      case '%':
        if (fprintf(template_c, "%%%%") < 0) return false;
        break;

      default:
        if (isprint(c)) {
          if (fputc(c, template_c) == EOF) return false;
        } else {
          printf("[ERROR] %s:%d: Invalid character '%c' (%d) in %s:%ld\n", __FILE__, __LINE__, c, c, html_filepath, line);
          return false;
        }
        break;

    }

    if (new_line || i == html_length-1) {
      if (fprintf(template_c, "\") < 0) return false; // %s:%d from %s:%s()\n", html_filepath, (int)line, __FILE__, __func__) < 0) return false;
    }
  }

  return true;
}

bool print_c_to_c_file(const char *html_filepath, long line_start, char *c_start, long c_length, FILE *template_c) {
  bool new_line = false;
  for (int i=0; i<c_length; i++) {
    const char c = c_start[i];
    switch (c) {
      case '\n':
        new_line = true;
        break;

      default:
        if (isprint(c)) {
          if (fputc(c, template_c) == EOF) return false;
        } else {
          printf("[ERROR] %s:%d: Invalid character '%c' (%d) in %s:%ld\n", __FILE__, __LINE__, c, c, html_filepath, line_start);
          return false;
        }
        break;

    }

    if (new_line || i == c_length-1) {
      if (fprintf(template_c, " // %s:%d from %s:%s()\n", html_filepath, (int)line_start, __FILE__, __func__) < 0) return false;
      new_line = false;
    }
  }

  return true;
}

bool print_html_template(const char *html_filepath, char *html, long html_size, FILE *template_c) {
  enum {
    STATE_HTML,
    STATE_C,
  } state = STATE_HTML;

  struct {
    long pos;
    long line;
  } start   = { .pos = 0, .line = 1 }, // Start of the current block of html or c code (depending of the state)
    current = { .pos = 0, .line = 1 }; // Current position and line in the html

  while (current.pos < html_size) {
    if (cstr_starts_with(html + current.pos, C_KEY_OPEN)) {
      switch (state) {
        case STATE_HTML:
          print_html_to_c_file(html_filepath, start.line, html + start.pos, current.pos - start.pos, template_c);
          state = STATE_C;
          break;

        case STATE_C:
          printf("[ERROR] %s:%d: Syntax error in the template file: Opening C tag (%s:%ld) inside C code (already opened at %s:%ld)\n", __FILE__, __LINE__, html_filepath, current.line, html_filepath, start.line);
          return false;
      }

      current.pos += strlen(C_KEY_OPEN);
      start.pos = current.pos;
      start.line = current.line;

    } else if (cstr_starts_with(html + current.pos, C_KEY_CLOSE)) {
      switch (state) {
        case STATE_HTML:
          printf("[ERROR] %s:%d: Syntax error in the template file: Mismatched closing C tag (%s:%ld)\n", __FILE__, __LINE__, html_filepath, current.line);
          return false;

        case STATE_C:
          print_c_to_c_file(html_filepath, start.line, html + start.pos, current.pos - start.pos, template_c);
          state = STATE_HTML;
          break;
      }

      current.pos += strlen(C_KEY_CLOSE);
      start.pos = current.pos;
      start.line = current.line;
    } else {
      if (html[current.pos] == '\n') current.line++;
      current.pos++;
    }
  }

  switch (state) {
    case STATE_HTML:
      print_html_to_c_file(html_filepath, start.line, html + start.pos, current.pos - start.pos, template_c);
      break;

    case STATE_C:
      printf("[ERROR] %s:%d: Syntax error in the template file: Mismatched opening C tag (%s:%ld)\n", __FILE__, __LINE__, html_filepath, start.line);
      return false;
  }

  return true;
}

bool generate_template_c(const char *html_filepath, char *html, long html_size, FILE *template_c) {
  if (!html) abort();
  if (!template_c) abort();

  COMMENT("***************************************");
  COMMENT("* DO NOT EDIT THIS FILE");
  COMMENT("* This file is generated by " __FILE__);
  COMMENT("***************************************");
  EMPTY_LINE();

  C_CODE("#include \"../../src/template/template.h\"");
  EMPTY_LINE();

  C_CODE("bool generate_html(FILE *f, List todo_list) {");

  if (!print_html_template(html_filepath, html, html_size, template_c)) return false;

  C_CODE("  return true;");
  C_CODE("}");

  return true;
}

int main(int argc, char *argv[]) {
  enum {
    SUCCESS,
    HELP,
    ERROR_FOPEN_HTML,
    ERROR_READING_HTML,
    ERROR_FOPEN_C_OUTPUT,
    ERROR_GENERATING_C_OUTPUT,
  } ret = SUCCESS;

  if (argc != 3 ) {
    printf("./%s [HTML template path] [output C file path]\n", argv[0]);
    DEFER(HELP);
  }

  const char *html_filepath = argv[1],
             *c_filepath = argv[2];

  FILE *file_html = fopen(html_filepath, "r");
  if (!file_html) DEFER(ERROR_FOPEN_HTML);

  char *template = NULL;
  long template_size = 0;
  if (!load_template(file_html, &template, &template_size)) DEFER(ERROR_READING_HTML);
  fclose(file_html); file_html = NULL;

  FILE *file_c = fopen(c_filepath, "w");
  if (!file_c) DEFER(ERROR_FOPEN_C_OUTPUT);

  if (!generate_template_c(html_filepath, template, template_size, file_c)) DEFER(ERROR_GENERATING_C_OUTPUT);

exit:
  switch (ret) {
    case SUCCESS: break;
    case HELP:    break;
    case ERROR_FOPEN_HTML:            printf("[ERROR] Unable to open the HTML file\n");                 break;
    case ERROR_READING_HTML:          printf("[ERROR] Unable to read the HTML file\n");                 break;
    case ERROR_FOPEN_C_OUTPUT:        printf("[ERROR] Unable to open the output C file for writing\n"); break;
    case ERROR_GENERATING_C_OUTPUT:   printf("[ERROR] Error while generating the output C file\n");     break;
  }

  if (file_html) fclose(file_html);
  if (file_c) fclose(file_c);
  if (ret == ERROR_GENERATING_C_OUTPUT) remove(c_filepath);
  if (template) free(template);
  return ret;
}
