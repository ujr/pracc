#include <assert.h>
#include <locale.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "daterange.h"
#include "scan.h"

#define streq(s,t) (strcmp((s),(t)) == 0)

/**
 * Given a symbolic name of a time period, compute
 * and return its start and end Unix time stamp:
 *
 *   toady      beginning of today till now
 *   thisweek   beginning of this week (Monday) till now
 *   thismonth  beginning of this month till now
 *   lastmonth  the last month, start to end
 *   thisyear   beginning of this year (January 1st)
 *   lastyear   the last year, start to end
 *   all        beginning of Unix time (Jan 1st, 1970) till now
 *   N          beginning of N days ago till now
 *
 * All other period names have the same effect as 'all'.
 */
void daterange(const char *period, time_t *tmin, time_t *tmax)
{
   time_t now, t0, t1;
   struct tm tm, *tmp;
   unsigned long days;
   int n;

   assert(period);

   if (tmin) *tmin = -1;
   if (tmax) *tmax = -1;

   now = time(0);
   tmp = localtime(&now);
   if (tmp) tm = *tmp;
   else return;
   tm.tm_isdst = -1; // unknown

   n = scanu(period, &days);
   if (n && (period[n] == '\0')) {
      tm.tm_sec = tm.tm_min = tm.tm_hour = 0;
      tm.tm_mday -= days; // mktime normalises
      t0 = mktime(&tm);
      t1 = now;
   }
   else if (streq(period, "thisyear")) {
      tm.tm_mday = 1; // 1st of...
      tm.tm_mon = 0; // January
      tm.tm_sec = tm.tm_min = tm.tm_hour = 0;
      t0 = mktime(&tm);
      t1 = now;
   }
   else if (streq(period, "lastyear")) {
      tm.tm_mday = 1; // 1st of...
      tm.tm_mon = 0; // January
      tm.tm_sec = tm.tm_min = tm.tm_hour = 0;
      t1 = mktime(&tm) - 1;
      tm.tm_year -= 1; // previous year
      tm.tm_isdst = -1;
      t0 = mktime(&tm);
   }
   else if (streq(period, "thismonth")) {
      tm.tm_mday = 1; // 1st of month
      tm.tm_sec = tm.tm_min = tm.tm_hour = 0;
      t0 = mktime(&tm);
      t1 = now;
   }
   else if (streq(period, "lastmonth")) {
      tm.tm_mday = 1; // 1st of month
      tm.tm_sec = tm.tm_min = tm.tm_hour = 0;
      t1 = mktime(&tm) - 1;
      if (tm.tm_mon > 0) tm.tm_mon -= 1;
      else { tm.tm_mon = 11; tm.tm_year -= 1; }
      tm.tm_isdst = -1;
      t0 = mktime(&tm);
   }
   else if (streq(period, "thisweek")) {
      days = tm.tm_wday;
      if (--days < 0) days = 6; // Mon=0
      tm.tm_sec = tm.tm_min = tm.tm_hour = 0;
      tm.tm_mday -= days; // mktime normalises
      t0 = mktime(&tm);
      t1 = now;
   }
   else if (streq(period, "today")) {
      tm.tm_sec = tm.tm_min = tm.tm_hour = 0;
      t0 = mktime(&tm);
      t1 = now;
   }
   else { // everything else means "all"
      t0 = 0;
      t1 = now;
   }

   if (tmin) *tmin = t0;
   if (tmax) *tmax = t1;
//fprintf(stderr, "daterange(%s): min %s\n", period, ctime(&t0)); //DEBUG
//fprintf(stderr, "daterange(%s): max %s\n", period, ctime(&t1)); //DEBUG
}
