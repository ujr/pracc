#include <assert.h>
#include <fnmatch.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "pracc.h"
#include "ui_pracclog.h"

#define OK (0)
#define FAIL (-1)
#define MAX(x,y) ((x)>(y)?(x):(y))

static struct pracclog logbuf;
static time_t mintime, maxtime;
static const char *acctpat;
static const char *typepat;
static long entrycount = -1; // unknown
static long firstindex, lastindex;

/*
 * Prepare for subsequent calls to pracclog_get().
 *
 * Return 0 if ok, -1 on error (with errno set)
 */
int pracclog_init(long first, long count, time_t tmin, time_t tmax,
                  const char *filter, const char *types)
{
   entrycount = -1; // unknown
   firstindex = MAX(first,1);
   lastindex = first + MAX(count,0) - 1;

   mintime = tmin;
   maxtime = tmax;

   /* Empty filter/types means all entries/types */
   acctpat = (filter && *filter) ? filter : 0;
   typepat = (types && *types) ? types : 0;

   (void) praccLogClose(&logbuf);
   return praccLogOpen(&logbuf);
}

/*
 * Return the number of matching pracc log entries.
 * Return -1 if unknown.
 */
long pracclog_count(void)
{
   return entrycount;
}

/*
 * Return the next log entry that matches the min/max, types,
 * and filter/pattern settings from a previous pracclog_init().
 *
 * Note that the pointers returned point to variables that
 * will be overwritten with the next call to pracclog_get().
 *
 * Note: The index argument is ignored!
 *
 * Return 0 if ok, 1 if no more entries,
 * and -1 on any other error (with errno set).
 */
int pracclog_get(int index, time_t *t, char **userp,
                 char **acctp, char **infop)
{
   int n;

   while ((n = praccLogRead(&logbuf)) > 0) {
      /* Compare against query criteria: */
      if ((mintime >= 0) && (logbuf.tstamp < mintime)) continue;
      if ((maxtime >= 0) && (maxtime < logbuf.tstamp)) return 1;
      if (!acctpat || fnmatch(acctpat, logbuf.acctname, 0)) continue;
      if (typepat && *typepat && !strchr(typepat, '*')
          && !strchr(typepat, logbuf.type)) continue;

      /* Compare against first/count: */
      if (entrycount < 0) entrycount = 1; else ++entrycount;
      if (entrycount < firstindex) continue;
      if (entrycount > lastindex) return 1;

      if (t) *t = logbuf.tstamp;
      if (userp) *userp = logbuf.username;
      if (acctp) *acctp = logbuf.acctname;
      if (infop) *infop = logbuf.infostr;

      return OK;
   }

   return (n < 0) ? -1 : 1;
}

/*
 * Dump all pracc.log records that match the given
 * date/filter/types settings to the given stream.
 *
 * Return 0 if ok, -1 on error (with errno set).
 */
int pracclog_dump(FILE *out, time_t tmin, time_t tmax,
                  const char *filter, const char *types)
{
   time_t t;
   char *user, *acct, *info;
   int r;

   assert(out);

   if (pracclog_init(1, LONG_MAX, tmin, tmax, filter, types) < 0)
      return -1; // see errno

   while ((r = pracclog_get(0, &t, &user, &acct, &info)) == 0) {
      struct tm *tmp;
      char buf[64];
      tmp = localtime(&t);
      if (tmp) strftime(buf, sizeof(buf), "%Y-%m-%d %H%M", tmp);
      else snprintf(buf, sizeof(buf), "YYYY-mm-dd HHMM");

      fprintf(out, "%s\t%s\t%s\t\"%s\"\n", buf, user, acct, info);
   }
   return (r < 0) ? -1 : 0;
}
