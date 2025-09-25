#ifndef STRINGS_H
#define STRINGS_H

#include <stdio.h>
#include <stdbool.h>

typedef struct {
  char *s;
  unsigned int len;
  unsigned int size;
} String_builder;

#define str_new() (String_builder) {0}

#define cstr_starts_with(str, start) (!strncmp(start, str, strlen(start)))

bool str_append_from_shell_variable(String_builder *str, const char *variable);

// Append a '/' at the end of str if it doesn't have it, and then append rel_path
void str_append_to_path(String_builder *str, char *rel_path);

void str_append(String_builder *str, const char *cstr);

void str_append_with_format(String_builder *sb, const char *fmt, ...);

String_builder str_create(const char *fmt, ...);

// str_dst and str_src SHOULD NOT be the same String_builder
void str_append_str(String_builder *str_dst, const String_builder str_src);

void str_append_int(String_builder *str, int n);

void str_append_uint(String_builder *str, unsigned int n);

void str_append_long(String_builder *str, long n);

void str_replace(String_builder *str, unsigned int index, const char *replace_cstr);

unsigned int str_length(String_builder str);

bool str_is_empty(String_builder str);

char *str_to_cstr(String_builder str);

void str_free(String_builder *str);

void str_clean(String_builder *str);

bool str_read_line(FILE *f, String_builder *str);

bool str_equals(String_builder sb1, String_builder sb2);

#endif // STRINGS_H
