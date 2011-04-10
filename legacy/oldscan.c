/* pracc v1 legacy */

#include <time.h>  /* struct tm */
#include "scan.h"  /* scandate etc */
#include "pracc.h" /* oldscan prototype */
#include "tai.h"

int oldscan(const char *s, struct tai *taip)
{ /* scan an old pracc v1 timestamp from s into tp */
  /* [date] yyyy-mm-dd [time] hh:mm:ss */
  /* @yyyy-mm-ddThh:mm:ssZ */
  register const char *p;
  struct tm tm;
  time_t t;
  int n;

  p = s;
  if (*p == '@') {
    ++p;
    if (n = scandate(p, &tm)) p += n;
    else return 0;
    if (*p++ != 'T') return 0;
    if (n = scantime(p, &tm)) p += n;
    else return 0;
    if (*p++ != 'Z') return 0;
    tm.tm_hour += 1; // UTC to CET
  }
  else {
    if (n = scans(p, "date")) p += n;
    while (isspace(*p)) ++p;
    if ((n = scandate(p, &tm)) == 0) return 0;
    p += n;
    while (isspace(*p)) ++p;
    if (n = scans(p, "time")) p += n;
    while (isspace(*p)) ++p;
    if ((n = scantime(p, &tm)) == 0) return 0;
    p += n;
    if (*p && !isspace(*p)) return 0;
  }

  tm.tm_wday = tm.tm_yday = tm.tm_isdst = 0;
  t = mktime(&tm);
  if (t == -1) return 0;
  if (taip) unixtai(taip, t);

  return p - s; /* #bytes scanned */
}
