#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include <libgen.h>
#include <stdlib.h>
#include <linux/limits.h>
#include <errno.h>

#include "../utils/string.h"

// Inspired from: <https://github.com/rexim/tore/blob/main/src_build/tt.c>

#define C_KEY_OPEN "<c>"
#define C_KEY_CLOSE "</c>"

#define COMMENT(output_c, line) if (fprintf(output_c, "// " line "\n") < 0) return false
#define C_CODE(output_c, line)  if (fprintf(output_c, "%s // " __FILE__ ":%d\n", line, __LINE__) < 0) return false
#define C_CODE_FMT(output_c, fmt, ...)  if (fprintf(output_c, fmt " // " __FILE__ ":%d\n", __VA_ARGS__, __LINE__) < 0) return false
#define EMPTY_LINE(output_c)  if (fprintf(output_c, "// " __FILE__ ":%d\n", __LINE__) < 0) return false

bool load_template(FILE *template_file, char **template_arr, long *template_arr_size) {
  if (!template_file) abort();
  if (!template_arr) abort();
  if (*template_arr) abort();

  if (fseek(template_file, 0, SEEK_END) == -1) return false;
  if ( (*template_arr_size = ftell(template_file)) == -1) return false;
  if (fseek(template_file, 0, SEEK_SET) == -1) return false;

  *template_arr = malloc(*template_arr_size+1);
  if ( !(*template_arr) ) return false;
  if (fread(*template_arr, *template_arr_size, 1, template_file) == 0) return false;
  (*template_arr)[*template_arr_size] = '\0';

  return true;
}

#define TEMPLATE_OPEN_FPRINTF(output_c) if (fprintf(output_c, "  if (fprintf(f, \"") < 0) return false
#define TEMPLATE_CLOSE_FPRINTF(output_c, filepath) if (fprintf(output_c, "\") < 0) FPRINTF_ERROR(); // %s:%d from %s:%s()\n", filepath, (int)line, __FILE__, __func__) < 0) return false

bool print_template_to_c_file(const char *template_filepath, long line, char *template_start, long template_length, FILE *output_c, bool *last_template_line_was_closed) {
  bool open_template_fprintf = true;

  // This means that a C tag was closed at the end of the line, so we break the
  // line if the line before the opening C tag (the last TEMPLATE line from the last
  // chunk of TEMPLATE code) doesn't have a line break at the end
  if (template_start[0] == '\n' && !(*last_template_line_was_closed)) {
    bool is_there_content = false;
    for (int x=1; x < template_length; x++) {
      if (template_start[x] != ' ' && template_start[x] != '\n') {
        is_there_content = true;
        break;
      }
    }

    if (is_there_content) {
      TEMPLATE_OPEN_FPRINTF(output_c);
      if (fprintf(output_c, "\\n") < 0) return false;
      TEMPLATE_CLOSE_FPRINTF(output_c, template_filepath);

      *last_template_line_was_closed = true;
    }
  }

  for (int i=0; i<template_length; i++) {
    const char c = template_start[i];

    if (open_template_fprintf) {
      if (c == ' ' && (*last_template_line_was_closed)) { // Skip leading spaces
        continue;
      }

      if (c == '\n') {
        line++;
        continue;
      }

      TEMPLATE_OPEN_FPRINTF(output_c);
      open_template_fprintf = false;
    }

    switch (c) {
      case '\n':
        if (fprintf(output_c, "\\n") < 0) return false;
        open_template_fprintf = true;
        break;

      case '"':
        if (fprintf(output_c, "\\\"") < 0) return false;
        break;

      case '%':
        if (fprintf(output_c, "%%%%") < 0) return false;
        break;

      case '\\':
        if (fprintf(output_c, "\\\\") < 0) return false;
        break;

      default:
        if (isprint(c)) {
          if (fputc(c, output_c) == EOF) return false;
        } else {
          printf("[ERROR] %s:%d: Invalid character '%c' (%d) in %s:%ld\n", __FILE__, __LINE__, c, c, template_filepath, line);
          return false;
        }
        break;
    }

    // Close the fprintf
    if (open_template_fprintf || i == template_length-1) {
      TEMPLATE_CLOSE_FPRINTF(output_c, template_filepath);
      line++;
    }

    *last_template_line_was_closed = open_template_fprintf;
  }

  return true;
}

bool print_c_to_c_file(const char *template_filepath, long line, char *c_start, long c_length, FILE *output_c) {
  bool new_line = true;
  bool end_line = false;

  for (int i=0; i<c_length; i++) {
    const char c = c_start[i];

    switch (c) {
      case '\n':
        if (new_line) { // Skip empty lines
          line++;
          continue;
        }
        end_line = true;
        new_line = true;
        break;

      case ' ':
        if (new_line) continue; // Skip leading spaces
        if (fputc(c, output_c) == EOF) return false;
        new_line = false;
        break;

      default:
        if (isprint(c)) {
          if (fputc(c, output_c) == EOF) return false;
        } else {
          printf("[ERROR] %s:%d: Invalid character '%c' (%d) in %s:%ld\n", __FILE__, __LINE__, c, c, template_filepath, line);
          return false;
        }
        new_line = false;
        break;

    }

    if (end_line || i == c_length-1) {
      if (fprintf(output_c, " // %s:%ld from %s:%s()\n", template_filepath, line, __FILE__, __func__) < 0) return false;
      end_line = false;
      line++;
    }
  }

  return true;
}

bool parse_template(const char *template_filepath, char *template, long template_size, FILE *output_c) {
  enum {
    STATE_TEMPLATE,
    STATE_C,
  } state = STATE_TEMPLATE;

  struct {
    long pos;
    long line;
  } start   = { .pos = 0, .line = 1 }, // Start of the current block of template or c code (depending of the state)
    current = { .pos = 0, .line = 1 }; // Current position and line in the template
  bool last_template_line_was_closed = true;

  while (current.pos < template_size) {
    if (cstr_starts_with(template + current.pos, C_KEY_OPEN)) {
      switch (state) {
        case STATE_TEMPLATE:
          print_template_to_c_file(template_filepath, start.line, template + start.pos, current.pos - start.pos, output_c, &last_template_line_was_closed);
          state = STATE_C;
          break;

        case STATE_C:
          printf("[ERROR] %s:%d: Syntax error in the template file: Opening C tag (%s:%ld) inside C code (already opened at %s:%ld)\n", __FILE__, __LINE__, template_filepath, current.line, template_filepath, start.line);
          return false;
      }

      current.pos += strlen(C_KEY_OPEN);
      start.pos = current.pos;
      start.line = current.line;

    } else if (cstr_starts_with(template + current.pos, C_KEY_CLOSE)) {
      switch (state) {
        case STATE_TEMPLATE:
          printf("[ERROR] %s:%d: Syntax error in the template file: Mismatched closing C tag (%s:%ld)\n", __FILE__, __LINE__, template_filepath, current.line);
          return false;

        case STATE_C:
          print_c_to_c_file(template_filepath, start.line, template + start.pos, current.pos - start.pos, output_c);
          state = STATE_TEMPLATE;
          break;
      }

      current.pos += strlen(C_KEY_CLOSE);
      start.pos = current.pos;
      start.line = current.line;
    } else {
      if (template[current.pos] == '\n') current.line++;
      current.pos++;
    }
  }

  switch (state) {
    case STATE_TEMPLATE:
      print_template_to_c_file(template_filepath, start.line, template + start.pos, current.pos - start.pos, output_c, &last_template_line_was_closed);
      break;

    case STATE_C:
      printf("[ERROR] %s:%d: Syntax error in the template file: Mismatched opening C tag (%s:%ld)\n", __FILE__, __LINE__, template_filepath, start.line);
      return false;
  }

  return true;
}

char *get_relative_path(const char *from, const char *to) {
  char abs_from[PATH_MAX], abs_to[PATH_MAX];

  if (!realpath(from, abs_from)) {
    printf("[ERROR] Unable to get the absolute path from `%s`: %s", from, strerror(errno));
    return NULL;
  }

  if (!realpath(to, abs_to)) {
    printf("[ERROR] Unable to get the absolute path from `%s`: %s", to, strerror(errno));
    return NULL;
  }

  String_builder sb = sb_new();

  int base_pos = 0;
  while ( abs_from[base_pos] && abs_to[base_pos] && abs_from[base_pos] == abs_to[base_pos] ) base_pos++;

  const size_t abs_from_len = strlen(abs_from);
  for (size_t i=base_pos; i < abs_from_len; i++) {
    if (abs_from[i] == '/') sb_append(&sb, "../");
  }
  sb_append(&sb, abs_to + base_pos);

  return sb.str;
}

bool generate_template_c(char *template_filepath, const char *c_filepath, char *template, long template_size, FILE *output_c) {
  if (!template_filepath) abort();
  if (!c_filepath) abort();
  if (!template) abort();
  if (!output_c) abort();

  COMMENT(output_c, "***************************************");
  COMMENT(output_c, "* DO NOT EDIT THIS FILE");
  COMMENT(output_c, "* This file is generated by " __FILE__);
  COMMENT(output_c, "***************************************");
  EMPTY_LINE(output_c);

  char *header_filepath = sb_create("%s.h", template_filepath).str;
  char *header_absolute_path = get_relative_path(c_filepath, header_filepath);
  free(header_filepath); header_filepath = NULL;
  if (!header_absolute_path) return false;

  C_CODE_FMT(output_c, "#include \"%s\"", header_absolute_path); // It can leak header_absolute_path, but I don't care
  free(header_absolute_path);
  C_CODE(output_c, "INITIALIZE_GLOBAL_VARS();");

  const char *template_filename = basename(template_filepath);
  C_CODE_FMT(output_c, "bool generate_%s(FILE *f) {", template_filename);

  if (!parse_template(template_filepath, template, template_size, output_c)) return false;

  C_CODE(output_c, "  return true;");
  C_CODE(output_c, "}");

  return true;
}

#define DEFER(value) do { \
    ret = value;          \
    goto exit;            \
  } while(0)

int main(int argc, char *argv[]) {
  enum {
    SUCCESS,
    HELP,
    ERROR_FOPEN_TEMPLATE,
    ERROR_READING_TEMPLATE,
    ERROR_FOPEN_C_OUTPUT,
    ERROR_GENERATING_C_OUTPUT,
  } ret = SUCCESS;
  FILE *file_template = NULL;
  FILE *file_c = NULL;
  char *template = NULL;
  // const char *template_name = NULL;
  char *template_path = NULL;
  char *c_filepath = NULL;

  if (argc != 3 ) {
    printf("%s [TEMPLATE filepath] [C OUTPUT filepath]\n", argv[0]);
    DEFER(HELP);
  }

  template_path = argv[1];
  c_filepath = argv[2];

  file_template = fopen(template_path, "r");
  if (!file_template) DEFER(ERROR_FOPEN_TEMPLATE);

  long template_size = 0;
  if (!load_template(file_template, &template, &template_size)) DEFER(ERROR_READING_TEMPLATE);
  fclose(file_template); file_template = NULL;

  file_c = fopen(c_filepath, "w");
  if (!file_c) DEFER(ERROR_FOPEN_C_OUTPUT);

  if (!generate_template_c(template_path, c_filepath, template, template_size, file_c)) DEFER(ERROR_GENERATING_C_OUTPUT);
  fclose(file_c); file_c = NULL;

exit:
  switch (ret) {
    case SUCCESS: break;
    case HELP:    break;
    case ERROR_FOPEN_TEMPLATE:        printf("[ERROR] Unable to open the TEMPLATE file `%s`\n", template_path); break;
    case ERROR_READING_TEMPLATE:      printf("[ERROR] Unable to read the TEMPLATE file `%s`\n", template_path); break;
    case ERROR_FOPEN_C_OUTPUT:        printf("[ERROR] Unable to open the output C file for writing\n");         break;
    case ERROR_GENERATING_C_OUTPUT:   printf("[ERROR] Error while generating the output C file\n");             break;
  }

  if (file_template) fclose(file_template);
  if (file_c) fclose(file_c);
  if (ret == ERROR_GENERATING_C_OUTPUT) remove(c_filepath);
  if (template) free(template);
  return ret;
}
