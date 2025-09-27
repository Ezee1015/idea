#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "string.h"

int basic_pow(int base, unsigned int exp) {
  int result = 1;
  while (exp) {
    result *= base;
    exp--;
  }
  return result;
}

void _sb_resize_if_necessary(String_builder *sb, unsigned int new_length) {
  unsigned int minimum_size = new_length + 1;
  if (minimum_size <= sb->_size) return;

  const int base = 2;
  const int minimum_exponent = 5;
  bool is_new = (sb->_size == 0);

  if (is_new) sb->_size = basic_pow(base, minimum_exponent-1);
  unsigned int new_size = sb->_size;
  while ( (new_size *= base) < minimum_size );
  sb->str = realloc(sb->str, new_size);
  if (!sb->str) abort();

  if (is_new) sb->str[0] = '\0';
  sb->_size = new_size;
}

void _sb_append(String_builder *sb, const char *cstr, unsigned int cstr_length) {
  if (!sb) abort();
  if (!cstr) abort();

  unsigned int new_length = sb->length + cstr_length;
  _sb_resize_if_necessary(sb, new_length);

  strcpy(sb->str + sb->length, cstr);
  sb->length = new_length;
}

void _sb_append_fmt(String_builder *sb, const char *fmt, va_list args) {
  unsigned int i = 0;
  bool finished = false;
  while (!finished) {
    switch (fmt[i]){
      case '%':
        switch (fmt[i+1]) {
          case '\0': finished = true; break;
          case '%': sb_append(sb, "%"); break;
          case 'u': sb_append_uint(sb, va_arg(args, unsigned int)); break;
          case 's': sb_append(sb, va_arg(args, char *)); break;
          case 'd': sb_append_int(sb, va_arg(args, int)); break;
          case 'l': // %ld
            if (fmt[i+2] != 'd') abort();
            sb_append_long(sb, va_arg(args, long));
            i++;
            break;

          default: abort();
        }
        i++;
        break;

      case '\0': finished = true; break;

      default: sb_append(sb, (char[2]){fmt[i], '\0'}); break;
    }
    i++;
  }
}

String_builder sb_create(const char *fmt, ...) {
  String_builder sb = sb_new();

  va_list args;
  va_start(args, fmt);
  _sb_append_fmt(&sb, fmt, args);
  va_end(args);

  return sb;
}

void sb_append_with_format(String_builder *sb, const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  _sb_append_fmt(sb, fmt, args);
  va_end(args);
}

bool sb_append_from_shell_variable(String_builder *sb, const char *variable) {
  const char *directory_path = getenv(variable);
  if (!directory_path) return false;

  sb_append(sb, directory_path);
  return true;
}

void sb_append(String_builder *sb, const char *cstr) {
  _sb_append(sb, cstr, strlen(cstr));
}

void sb_append_str(String_builder *sb_dst, const String_builder sb_src) {
  _sb_append(sb_dst, sb_src.str, sb_src.length);
}

void sb_append_c(String_builder *sb, char c) {
  char cstr[2] = {c, '\0'};
  _sb_append(sb, cstr, 1);
}

void sb_append_int(String_builder *sb, int n) {
  unsigned int n_length = (n <= 0) ? 1 : 0;
  int aux = (n > 0) ? n : -1 * n;
  while (aux > 0) {
    aux /= 10;
    n_length++;
  }

  unsigned int new_length = sb->length + n_length;
  _sb_resize_if_necessary(sb, new_length);

  sprintf(sb->str + sb->length, "%d", n);
  sb->length = new_length;
}

void sb_append_long(String_builder *sb, long n) {
  unsigned int n_length = (n <= 0) ? 1 : 0;
  long aux = (n > 0) ? n : -1 * n;
  while (aux > 0) {
    aux /= 10;
    n_length++;
  }

  unsigned int new_length = sb->length + n_length;
  _sb_resize_if_necessary(sb, new_length);

  sprintf(sb->str + sb->length, "%ld", n);
  sb->length = new_length;
}

void sb_append_uint(String_builder *sb, unsigned int n) {
  unsigned int n_length = (n == 0) ? 1 : 0;
  unsigned int aux = n;
  while (aux > 0) {
    aux /= 10;
    n_length++;
  }

  unsigned int new_length = sb->length + n_length;
  _sb_resize_if_necessary(sb, new_length);

  sprintf(sb->str + sb->length, "%u", n);
  sb->length = new_length;
}

void sb_replace(String_builder *sb, unsigned int index, const char *replace_cstr) {
  if (index >= sb->length) abort();

  unsigned int new_length = index + strlen(replace_cstr);
  _sb_resize_if_necessary(sb, new_length);

  strcpy(sb->str + index, replace_cstr);
}

bool sb_is_empty(String_builder sb) {
  return sb.length == 0;
}

void sb_free(String_builder *sb) {
  sb->length = 0;
  sb->_size = 0;
  if (sb->str) free(sb->str);
  sb->str = NULL;
}

void sb_clean(String_builder *sb) {
  sb->length = 0;
  if (sb->str) sb->str[0] = '\0';
}

bool sb_read_line(FILE *f, String_builder *sb) {
  if (!f) abort();
  if (!sb) abort();

  char buffer[512];
  bool read = false;
  bool line_finished = false;
  while (!line_finished && fgets(buffer, 512, f)) {
    read = true;
    unsigned int buffer_len = strlen(buffer);

    if (buffer[buffer_len-1] == '\n') {
      buffer[--buffer_len] = '\0';
      line_finished = true;
    }

    if (!buffer_len) break;

    _sb_append(sb, buffer, buffer_len);
  }

  return read;
}

bool sb_equals(String_builder sb1, String_builder sb2) {
  return (sb1.length == sb2.length && !strcmp(sb1.str, sb2.str));
}
}
