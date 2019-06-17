#include "scan.h"
#include <time.h>

/**
 * Scan the string s for a date in ISO 8601 format, eg, 2005-07-14.
 * Only the tm_year, tm_mon, tm_mday fields of the struct tm are updated;
 * all other fields are left alone.
 *
 * Notes on struct tm:
 * tm_year is the number of years since 1900.
 * tm_mon (0..11) is the number of months since January.
 * tm_mday (day of month) is in the normal range 1..31.
 *
 * Return number of chars scanned, zero on error.
 */
int scandate(const char *s, struct tm *t)
{
  register const char *p = s;
  unsigned long z, c;
  int sign = 1;

  if (*p == '-') { ++p; sign = -1; }
  z = 0; while ((c = (unsigned char) (*p - '0')) <= 9) { z = z*10 + c; ++p; }
  t->tm_year = z * sign - 1900;

  if (*p++ != '-') return 0;
  z = 0; while ((c = (unsigned char) (*p - '0')) <= 9) { z = z*10 + c; ++p; }
  t->tm_mon = z - 1;

  if (*p++ != '-') return 0;
  z = 0; while ((c = (unsigned char) (*p - '0')) <= 9) { z = z*10 + c; ++p; }
  t->tm_mday = z;

  return p - s;  /* #chars scanned */
}
