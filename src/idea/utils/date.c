#include <stdlib.h>
#include <time.h>
#include <string.h>

#include "date.h"
#include "../../utils/string.h"
#include "../../utils/tokenizer.h"
#include "backtrace.h"

Date date_now() {
  time_t now       = time(NULL);
  struct tm *now_t = localtime(&now);

  return (Date) {
    .year  = now_t->tm_year + 1900,
    .month = now_t->tm_mon + 1,
    .day   = now_t->tm_mday,
  };
}

int get_delta_time_days(Date date_from, Date date_to) {
  // Source: <https://stackoverflow.com/a/73846054>
  struct tm date_from_tm = {
    .tm_year = date_from.year - 1900,
    .tm_mon = date_from.month - 1,
    .tm_mday = date_from.day,
    .tm_isdst = -1,
  };

  struct tm date_to_tm = {
    .tm_year = date_to.year - 1900,
    .tm_mon = date_to.month - 1,
    .tm_mday = date_to.day,
    .tm_isdst = -1,
  };

  time_t date_from_t = mktime(&date_from_tm);
  time_t date_to_t = mktime(&date_to_tm);

  double dt = difftime(date_to_t, date_from_t);
  int days = dt / 86400;

  return days;
}

char *get_delta_time_string(Date date_from, Date date_to) {
  int days = get_delta_time_days(date_from, date_to);
  char *when = (days < 0) ? "ago" : "left";
  days = abs(days);

  if (days == 0) return sb_create("0 days left").str;

  int y = days/365; // It floors it automatically
  int d_of_y = y * 365; // Days from the amount of years

  int m = (days - d_of_y)/30;
  int d_of_m = m * 30;

  int w = (days - d_of_y - d_of_m)/7;
  int d_of_w = w*7;

  int d = days - d_of_y - d_of_m - d_of_w;

  String_builder sb = sb_new();

  if (y) sb_append_with_format(&sb, "%d year%s ", y, (y != 1) ? "s" : "");
  if (m) sb_append_with_format(&sb, "%d month%s ", m, (m != 1) ? "s" : "");
  if (w) sb_append_with_format(&sb, "%d week%s ", w, (w != 1) ? "s" : "");
  if (d) sb_append_with_format(&sb, "%d day%s ", d, (d != 1) ? "s" : "");
  sb_append(&sb, when);

  return sb.str;
}

bool is_date_equals(Date date_1, Date date_2) {
  return (date_1.year == date_2.year && date_1.month == date_2.month && date_1.day == date_2.day);
}

bool is_date_greater(Date date_greater, Date date_less) {
  return get_delta_time_days(date_less, date_greater) > 0;
}

bool is_date_less(Date date_less, Date date_greater) {
  return is_date_greater(date_greater, date_less);
}

bool load_date_from_string(char *date_str, Date *date) {
  if (!date_str || !date) return false;

  Input date_input = {
    .input = date_str,
    .length = strlen(date_str),
    .cursor = 0,
  };

  // Year
  char *rem_year_str = next_token(&date_input, '-');
  if (!rem_year_str) {
    APPEND_TO_BACKTRACE(BACKTRACE_ERROR, "Unable to get the year from the date '%s'", date_str);
    return false;
  }

  date->year = atoi(rem_year_str);
  free(rem_year_str);
  if (date->year == 0) {
    APPEND_TO_BACKTRACE(BACKTRACE_ERROR, "Unable to parse the year from the date '%s'", date_str);
    return false;
  }

  // Month
  char *rem_month_str = next_token(&date_input, '-');
  if (!rem_month_str) {
    APPEND_TO_BACKTRACE(BACKTRACE_ERROR, "Unable to get the month from the date '%s'", date_str);
    return false;
  }

  date->month = atoi(rem_month_str);
  free(rem_month_str);
  if (date->month == 0) {
    APPEND_TO_BACKTRACE(BACKTRACE_ERROR, "Unable to parse the month from the date '%s'", date_str);
    return false;
  }

  // day
  char *rem_day_str = next_token(&date_input, '-');
  if (!rem_day_str) {
    APPEND_TO_BACKTRACE(BACKTRACE_ERROR, "Unable to get the day from the date '%s'", date_str);
    return false;
  }

  date->day = atoi(rem_day_str);
  free(rem_day_str);
  if (date->day == 0) {
    APPEND_TO_BACKTRACE(BACKTRACE_ERROR, "Unable to parse the day from the date '%s'", date_str);
    return false;
  }

  return true;
}
