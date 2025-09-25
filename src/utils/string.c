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

void _str_resize_if_necessary(String_builder *str, unsigned int new_length) {
  unsigned int minimum_size = new_length + 1;
  if (minimum_size <= str->size) return;

  const int base = 2;
  const int minimum_exponent = 5;
  bool is_new = (str->size == 0);

  if (is_new) str->size = basic_pow(base, minimum_exponent-1);
  unsigned int new_size = str->size;
  while ( (new_size *= base) < minimum_size );
  str->s = realloc(str->s, new_size);
  if (!str->s) abort();

  if (is_new) str->s[0] = '\0';
  str->size = new_size;
}

void _str_append(String_builder *str, const char *cstr, unsigned int cstr_length) {
  if (!str) abort();
  if (!cstr) abort();

  unsigned int new_length = str->len + cstr_length;
  _str_resize_if_necessary(str, new_length);

  strcpy(str->s + str->len, cstr);
  str->len = new_length;
}

void _str_append_fmt(String_builder *sb, const char *fmt, va_list args) {
  unsigned int i = 0;
  bool finished = false;
  while (!finished) {
    switch (fmt[i]){
      case '%':
        switch (fmt[i+1]) {
          case '\0': finished = true; break;
          case '%': str_append(sb, "%"); break;
          case 'u': str_append_uint(sb, va_arg(args, unsigned int)); break;
          case 's': str_append(sb, va_arg(args, char *)); break;
          case 'd': str_append_int(sb, va_arg(args, int)); break;
          case 'l': // %ld
            if (fmt[i+2] != 'd') abort();
            str_append_long(sb, va_arg(args, long));
            i++;
            break;

          default: abort();
        }
        i++;
        break;

      case '\0': finished = true; break;

      default: str_append(sb, (char[2]){fmt[i], '\0'}); break;
    }
    i++;
  }
}

String_builder str_create(const char *fmt, ...) {
  String_builder sb = str_new();

  va_list args;
  va_start(args, fmt);
  _str_append_fmt(&sb, fmt, args);
  va_end(args);

  return sb;
}

void str_append_with_format(String_builder *sb, const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  _str_append_fmt(sb, fmt, args);
  va_end(args);
}

bool str_append_from_shell_variable(String_builder *str, const char *variable) {
  const char *directory_path = getenv(variable);
  if (!directory_path) return false;

  str_append(str, directory_path);
  return true;
}

void str_append_to_path(String_builder *str, char *rel_path) {
  if (str->s[str->len-1] != '/') _str_append(str, "/", 1);
  _str_append(str, rel_path, strlen(rel_path));
}

void str_append(String_builder *str, const char *cstr) {
  _str_append(str, cstr, strlen(cstr));
}

void str_append_str(String_builder *str_dst, const String_builder str_src) {
  _str_append(str_dst, str_src.s, str_src.len);
}

void str_append_c(String_builder *str, char c) {
  char cstr[2] = {c, '\0'};
  _str_append(str, cstr, 1);
}

void str_append_int(String_builder *str, int n) {
  unsigned int n_length = (n <= 0) ? 1 : 0;
  int aux = (n > 0) ? n : -1 * n;
  while (aux > 0) {
    aux /= 10;
    n_length++;
  }

  unsigned int new_length = str->len + n_length;
  _str_resize_if_necessary(str, new_length);

  sprintf(str->s + str->len, "%d", n);
  str->len = new_length;
}

void str_append_long(String_builder *str, long n) {
  unsigned int n_length = (n <= 0) ? 1 : 0;
  long aux = (n > 0) ? n : -1 * n;
  while (aux > 0) {
    aux /= 10;
    n_length++;
  }

  unsigned int new_length = str->len + n_length;
  _str_resize_if_necessary(str, new_length);

  sprintf(str->s + str->len, "%ld", n);
  str->len = new_length;
}

void str_append_uint(String_builder *str, unsigned int n) {
  unsigned int n_length = (n == 0) ? 1 : 0;
  unsigned int aux = n;
  while (aux > 0) {
    aux /= 10;
    n_length++;
  }

  unsigned int new_length = str->len + n_length;
  _str_resize_if_necessary(str, new_length);

  sprintf(str->s + str->len, "%u", n);
  str->len = new_length;
}

void str_replace(String_builder *str, unsigned int index, const char *replace_cstr) {
  if (index >= str->len) abort();

  unsigned int new_length = index + strlen(replace_cstr);
  _str_resize_if_necessary(str, new_length);

  strcpy(str->s + index, replace_cstr);
}

unsigned int str_length(String_builder str) {
  return str.len;
}

bool str_is_empty(String_builder str) {
  return str.len == 0;
}

char *str_to_cstr(String_builder str) {
  return str.s;
}

void str_free(String_builder *str) {
  str->len = 0;
  str->size = 0;
  if (str->s) free(str->s);
  str->s = NULL;
}

void str_clean(String_builder *str) {
  str->len = 0;
  if (str->s) str->s[0] = '\0';
}

bool str_read_line(FILE *f, String_builder *str) {
  if (!f) abort();
  if (!str) abort();

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

    _str_append(str, buffer, buffer_len);
  }

  return read;
}

bool str_equals(String_builder sb1, String_builder sb2) {
  return (sb1.len == sb2.len && !strcmp(sb1.s, sb2.s));
}
