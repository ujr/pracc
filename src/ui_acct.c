#include <assert.h>
#include <errno.h>
#include <string.h>
#include <time.h>

#include "pracc.h"
#include "webgui.h"

#define OK (0)
#define FAIL (-1)
#define MAX(x,y) ((x)>(y)?(x):(y))

static struct praccbuf praccbuf;
static time_t mintime, maxtime;
static const char *typepat;
static long entrycount = -1; // unknown
static long firstindex, lastindex;

/*
 * Prepare for subsequent calls to acct_get().
 *
 * Return 0 if ok, -1 on error (with errno set).
 */
int acct_init(long first, long count, const char *acctname,
              time_t tmin, time_t tmax, const char *types)
{
   int r;

   if (!acctname) {
      errno = EINVAL;
      return FAIL;
   }

   entrycount = -1; // unknown
   firstindex = MAX(first,1);
   lastindex = first + MAX(count,0) - 1;

   mintime = tmin;
   maxtime = tmax;
   typepat = types;

   (void) praccClose(&praccbuf);
   r = praccOpen(acctname, &praccbuf);

   return r;
}

/*
 * Return the number of matching account entries.
 * Return -1 if unknown.
 */
long acct_count(void)
{
   return entrycount;
}

/*
 * Return the next matching account entry.
 *
 * Note that the pointers returned point to variables that
 * will be overwritten with the next call to this function.
 *
 * Note: The index argument is ignored!
 *
 * Return 0 if ok, 1 if no more entries,
 * and -1 on any other error (with errno set).
 */
int acct_get(int index, time_t *tstamp, char *type, long *value,
             char **user, char **comment)
{
   int n;

   if (!praccbuf.fp) return FAIL;
   while ((n = praccRead(&praccbuf)) > 0) {
      /* Compare against query criteria: */
      if ((mintime >= 0) && (praccbuf.tstamp < mintime)) continue;
      if ((maxtime >= 0) && (maxtime < praccbuf.tstamp)) return 1;
      if (praccbuf.type == '#') continue; // no notes!
      if (typepat && typepat[0] && // empty: keep
          !strchr(typepat, praccbuf.type)) continue;

      /* Compare against first/count: */
      if (entrycount < 0) entrycount = 1; else ++entrycount;
      if (entrycount < firstindex) continue;
      if (entrycount > lastindex) return 1;

      if (tstamp) *tstamp = praccbuf.tstamp;
      if (type) *type = praccbuf.type;
      if (value) *value = praccbuf.value;
      if (user) *user = praccbuf.username;
      if (comment) *comment = praccbuf.comment;

      return OK;
   }

   return (n < 0) ? -1 : 1;
}

int
acct_dump(FILE *out, const char *acctname,
          time_t tmin, time_t tmax, const char *types)
{
   int r;
   time_t tstamp;
   char type;
   long value;
   char *user;
   char *comment;
   double floatValue;

   assert(out != NULL);

   r = acct_init(0, 999999, acctname, tmin, tmax, types);
   if (r < 0) return FAIL; // see errno

   // Emit the header line:
   fprintf(out, "#Date\tTime\tUser\tWhat\tValue\tInformation\n");

   while (1) {
      r = acct_get(0, &tstamp, &type, &value, &user, &comment);
      if (r != OK) {
         if (r < 0 && errno != 0) {
            fprintf(out, "#Error processing: %s\n", strerror(errno));
         }
         // else if r > 0: no more pracc records
         break;
      }

      char datestr[32], timestr[32], *typestr;
      struct tm *tmp = localtime(&tstamp); 

      if (tmp) {
         strftime(datestr, sizeof(datestr), "%Y-%m-%d", tmp);
         strftime(timestr, sizeof(timestr), "%H:%M", tmp);
      }
      else {
         snprintf(datestr, sizeof(datestr), "YYYY-mm-dd");
         snprintf(timestr, sizeof(timestr), "HHMM");
      }

      if (!(typestr = praccTypeString(type))) {
         typestr = "unknown";
      }

      floatValue = value/100; // Rappen -> Franken

      fprintf(out, "%s\t%s\t%s\t%s\t%.02f\t%s\n",
              datestr, timestr, user, typestr, floatValue, comment);
   }

   return OK;
}
