#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "string.h"

#define number_length(type, number, length) do { \
  length = (n <= 0) ? 1 : 0;                     \
  type __aux = (n > 0) ? n : -1 * n;             \
  while (__aux > 0) {                            \
    __aux /= 10;                                 \
    length++;                                    \
  }                                              \
} while(0)

#define _sb_append_number(type, format_specifier, number) do { \
  unsigned int n_length;                                       \
  number_length(type, number, n_length);                       \
                                                               \
  unsigned int new_length = sb->length + n_length;             \
  _sb_resize_if_necessary(sb, new_length);                     \
                                                               \
  sprintf(sb->str + sb->length, format_specifier, n);          \
  sb->length = new_length;                                     \
} while (0)

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

#define pad_unsigned_number(type, number, size) do {  \
  unsigned int __length = 0;                          \
  number_length(type, number, __length);              \
  int __padding_left = size - __length;               \
  while (__padding_left > 0) {                        \
    sb_append_char(sb, fill);                         \
    __padding_left--;                                 \
  }                                                   \
} while (0)

// Implemented:
// - Left padding
// - %u
// - %s
// - %d
// - %ld
void _sb_append_fmt(String_builder *sb, const char *fmt, va_list args) {
  unsigned int i = 0;
  bool finished = false;

  while (!finished) {
    switch (fmt[i]){
      case '%': {
        i++;

        unsigned size = 0;
        char fill = ' ';

        while ('0' <= fmt[i] && fmt[i] <= '9') {
          if (size == 0 && fmt[i] == '0') fill = '0';
          size *= 10;
          size += fmt[i] - '0';
          i++;
        }

        switch (fmt[i]) {
          case '\0': finished = true; break;
          case '%': sb_append(sb, "%"); break;
          case 'u': {
            unsigned int n = va_arg(args, unsigned int);
            pad_unsigned_number(unsigned int, n, size);
            sb_append_uint(sb, n);
            break;
          }

          case 'd': {
            int n = va_arg(args, int);
            if (size) {
              if (n < 0) {
                n *= -1;
                size--;
                sb_append_char(sb, '-');
              }
              pad_unsigned_number(int, n, size);
            }
            sb_append_int(sb, n);
            break;
          }

          case 's': {
            char *str = va_arg(args, char *);
            int padding_left = size - strlen(str);
            while (padding_left > 0) {
              sb_append_char(sb, fill);
              padding_left--;
            }

            sb_append(sb, str);
            break;
          }

          case 'l': { // %ld
            i++;
            if (fmt[i] != 'd') abort();

            long n = va_arg(args, long);
            if (size) {
              if (n < 0) {
                n *= -1;
                size--;
                sb_append_char(sb, '-');
              }
              pad_unsigned_number(long, n, size);
            }
            sb_append_long(sb, n);
            break;
          }

          default: abort();
        }
        break;
      }

      case '\0': finished = true; break;

      default: sb_append_char(sb, fmt[i]); break;
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
  _sb_append_number(int, "%d", n);
}

void sb_append_long(String_builder *sb, long n) {
  _sb_append_number(long, "%ld", n);
}

void sb_append_uint(String_builder *sb, unsigned int n) {
  _sb_append_number(unsigned int, "%u", n);
}

void sb_append_char(String_builder *sb, char c) {
  sb_append(sb, (char[2]){c, '\0'});
}

void sb_insert_at(String_builder *sb, unsigned int index, const char *cstr) {
  if (!sb || index > sb->length || !cstr) abort();
  unsigned int cstr_length = strlen(cstr);

  unsigned int new_length = sb->length + cstr_length;
  _sb_resize_if_necessary(sb, new_length);

  for (int i = new_length; i >= (int) (index + cstr_length); i--) {
    sb->str[i] = sb->str[i - cstr_length];
  }

  strncpy(sb->str+index, cstr, cstr_length);
  sb->length = new_length;
}

void sb_remove(String_builder *sb, unsigned int index, unsigned int length) {
  if (!sb || index > sb->length) abort();

  unsigned int new_length = sb->length - length;
  _sb_resize_if_necessary(sb, new_length);

  strcpy(sb->str+index, sb->str+index+length);
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

bool sb_search_and_replace(String_builder *sb, const char *search, const char *replace) {
  const unsigned int search_length = strlen(search);
  const unsigned int replace_length = strlen(replace);
  bool found = false;

  for (unsigned int i=0; i<sb->length; i++) {
    if (cstr_starts_with(sb->str+i, search)) {
      sb_remove(sb, i, search_length);
      sb_insert_at(sb, i, replace);
      found = true;
      i += replace_length - search_length;
    }
  }

  return found;
}
