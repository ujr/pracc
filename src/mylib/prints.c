#include "print.h"

unsigned prints(char *s, const char *t)
{ /* print a string, return #chars printed */
  register const char *p = t;

  if (!p) return 0;
  if (s) while (*p) *s++ = *p++;
  else while (*p) p++;

  return p - t;
}
