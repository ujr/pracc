#include "scan.h"
#include <time.h>

/**
 * Scan the string s for a time in ISO 8601 format, eg, 12:34:56.
 * Only the tm_hour, tm_min, tm_sec fields of the struct tm are updated;
 * all other fields are left alone.
 *
 * TODO Scan optional time offset from GMT, eg Z or +0200
 *
 * Return number of chars scanned, zero on error.
 */
int scantime(const char *s, struct tm *t/*, long *offset*/)
{
  register const char *p = s;
  unsigned long c, z;

  z = 0;
  while ((c = (unsigned char) (*p - '0')) <= 9) { z = z*10 + c; ++p; }
  t->tm_hour = z;

  if (*p++ != ':') return 0;

  z = 0;
  while ((c = (unsigned char) (*p - '0')) <= 9) { z = z*10 + c; ++p; }
  t->tm_min = z;

  if (*p != ':') t->tm_sec = 0;
  else { ++p;
  	z = 0;
  	while ((c = (unsigned char) (*p - '0')) <= 9) { z = z*10 + c; ++p; }
  	t->tm_sec = z;
  }

#if 0
  if (!offset) return p - s;  /* #chars scanned */

  /* scan offset from GMT */
  while ((*p == ' ') || (*p == '\t')) ++p;
  if (*p == '+') sign = 1; else if (*p == '-') sign = -1; else return 0;
  ++p;
  c = (unsigned char) (*p++ - '0'); if (c > 9) return 0; z = c;
  c = (unsigned char) (*p++ - '0'); if (c > 9) return 0; z = z*10 + c;
  c = (unsigned char) (*p++ - '0'); if (c > 9) return 0; z = z*6 + c;
  c = (unsigned char) (*p++ - '0'); if (c > 9) return 0; z = z*10 + c;
  *offset = z * sign;
#endif

  return p - s;  /* #chars scanned */
}
