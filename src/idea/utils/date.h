#ifndef DATE_H
#define DATE_H

#include <stdbool.h>

typedef struct {
  int year;
  int month;
  int day;
} Date;

Date date_now();
int get_delta_time_days(Date date_from, Date date_to);
char *get_delta_time_string(Date date_from, Date date_to);
bool load_date_from_string(char *date_str, Date *date);

bool is_date_equals(Date date_1, Date date_2);
bool is_date_greater(Date date_greater, Date date_less);
bool is_date_less(Date date_less, Date date_greater);
#define is_date_less_or_equals(date_1, date_2) (is_date_less(date_1, date_2) || is_date_equals(date_1, date_2))
#define is_date_greater_or_equals(date_1, date_2) (is_date_greater(date_1, date_2) || is_date_equals(date_1, date_2))

#endif // DATE_H
