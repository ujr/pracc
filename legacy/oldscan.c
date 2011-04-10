/* pracc v1 legacy */

#include <time.h>  /* struct tm */
#include "scan.h"  /* scandate etc */
#include "pracc.h" /* oldscan prototype */

int oldscan(const char *s, struct tm *tp)
{ /* scan an old pracc v1 timestamp from s into tp */
  /* [date] yyyy-mm-dd [time] hh:mm:ss */
  register const char *p;
  struct tm tm;
  int n;

  p = s;
  if (n = scanfix(p, "date")) p += n;
  while (isspace(*p)) ++p;
  if ((n = scandate(p, &tm)) == 0) return 0;
  p += n;
  while (isspace(*p)) ++p;
  if (n = scanfix(p, "time")) p += n;
  while (isspace(*p)) ++p;
  if ((n = scantime(p, &tm)) == 0) return 0;
  p += n;
  if (*p && !isspace(*p)) return 0;

  tm.tm_wday = tm.tm_yday = tm.tm_isdst = 0;
  if (tp) *tp = tm;

  return p - s; /* #bytes scanned */
}
