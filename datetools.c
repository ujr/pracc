#include <assert.h>
#include <locale.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "datetools.h"
#include "scan.h"

#define streq(s,t) (strcmp((s),(t)) == 0)

/*
 * Parse date in yyyy-mm-dd format into a time_t.
 * Return number of characters scanned, 0 on error.
 */
int parsedate(const char *s, time_t *tp)
{
   struct tm tm;
   time_t t;
   int n;

   if (!s) return 0;
   if (!(n = scandate(s, &tm))) return 0;

   tm.tm_sec = tm.tm_min = tm.tm_hour = 0;
   if ((t = mktime(&tm)) < 0) return 0;

   if (tp) *tp = t;

   return n; // #chars scanned
}

/*
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
 * Return 0 if ok, -1 on error (probably with EINVAL
 * because of an unknown period name).
 */
int daterange(const char *period, time_t *tmin, time_t *tmax)
{
   time_t now, t0, t1;
   struct tm tm, *tmp;
   unsigned long days;
   int n, nowdst;

   assert(period);

   now = time(0);
   tmp = localtime(&now);
   if (tmp) tm = *tmp;
   else return -1;
   nowdst = tm.tm_isdst;
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
   else if (streq(period, "all")) {
      t0 = 0;
      t1 = now;
   }
   else { // everything else means "today"
      tm.tm_sec = tm.tm_min = tm.tm_hour = 0;
      t0 = mktime(&tm);
      t1 = now;
   }

   if (tmin) *tmin = t0;
   if (tmax) *tmax = t1;
//fprintf(stderr, "daterange(%s): min %s\n", period, ctime(&t0)); //DEBUG
//fprintf(stderr, "daterange(%s): max %s\n", period, ctime(&t1)); //DEBUG
   return ((t0 < 0) || (t1 < 0)) ? -1 : 0;
}

#if 0
/*
 * Return a unix timestamp that is before the given timestamp
 * (if NULL, current time) as specified by the period argument:
 *
 *   toady      beginning of today
 *   week       beginning of this week (Monday)
 *   month      beginning of this month
 *   year       beginning of this year (January 1st)
 *   all        beginning of Unix time (Jan 1st, 1970)
 *   N          beginning of N days ago
 *
 * Note: I've also an implementation of this in JavaScript.
 */
time_t backdate(time_t *tp, const char *period)
{
   time_t now, then;
   struct tm tm;
   unsigned long days;
   int n;

   assert(period);

   if (tp) now = *tp;
   else now = time(0);
   tm = *localtime(&now);
   tm.tm_isdst = 0;

   n = scanu(period, &days);
   if (n && (period[n] == '\0')) {
      tm.tm_sec = tm.tm_min = tm.tm_hour = 0;
      then = mktime(&tm);
      then -= days*24*60*60;
   }
   else if (streq(period, "year")) {
      tm.tm_mday = 1; // 1st of...
      tm.tm_mon = 0; // January
      tm.tm_sec = tm.tm_min = tm.tm_hour = 0;
      then = mktime(&tm);
   }
   else if (streq(period, "month")) {
      tm.tm_mday = 1; // 1st of month
      tm.tm_sec = tm.tm_min = tm.tm_hour = 0;
      then = mktime(&tm);
   }
   else if (streq(period, "week")) {
      days = tm.tm_wday;
      if (--days < 0) days = 6; // Mon=0
      tm.tm_sec = tm.tm_min = tm.tm_hour = 0;
      then = mktime(&tm);
      then -= days*24*60*60;
   }
   else if (streq(period, "all")) then = 0;
   else { // everything else means "today"
      tm.tm_sec = tm.tm_min = tm.tm_hour = 0;
      then = mktime(&tm);
   }

//fprintf(stderr, "backdate %s: %s", period, ctime(&then));//DEBUG
   return (then < 0) ? now : then;
}
#endif
