#ifndef STRINGS_H
#define STRINGS_H

#include <stdio.h>
#include <stdbool.h>

typedef struct {
  char *str;
  unsigned int length;
  unsigned int _size;
} String_builder;

#define sb_new() (String_builder) {0}

#define cstr_starts_with(cstr, start) (!strncmp(start, cstr, strlen(start)))

#define sb_inspect(sb) do {                                     \
  printf("================================================\n"); \
  printf(__FILE__ ":%d: -inside %s()- str_inspect(" #sb "):\n", __LINE__, __FUNCTION__); \
  printf("- Internal size: %u\n", sb._size);                    \
  printf("- Length: %u\n", sb.length);                          \
  printf("- String: |%s|\n", sb.str);                           \
                                                                \
  char *aux = sb.str;                                           \
  int i = 0;                                                    \
  while ( *aux ) {                                              \
    printf("          - [%d]: %c (%d)\n", i++, *aux, *aux);     \
    aux++;                                                      \
  }                                                             \
                                                                \
  printf("================================================\n"); \
} while (0)

bool sb_append_from_shell_variable(String_builder *sb, const char *variable);

void sb_append(String_builder *sb, const char *cstr);

void sb_append_with_format(String_builder *sb, const char *fmt, ...);

String_builder sb_create(const char *fmt, ...);

// sb_dst and sb_src SHOULD NOT be the same String_builder
void sb_append_str(String_builder *sb_dst, const String_builder sb_src);

void sb_append_int(String_builder *sb, int n);

void sb_append_uint(String_builder *sb, unsigned int n);

void sb_append_long(String_builder *sb, long n);

void sb_append_char(String_builder *sb, char c);

void sb_replace(String_builder *sb, unsigned int index, const char *replace_cstr);

bool sb_is_empty(String_builder sb);

void sb_free(String_builder *sb);

void sb_clean(String_builder *sb);

bool sb_read_line(FILE *f, String_builder *sb);

bool sb_equals(String_builder sb1, String_builder sb2);

#endif // STRINGS_H
