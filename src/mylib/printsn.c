#include "print.h"

unsigned printsn(char *s, const char *t, unsigned max)
{ /* print a string, no more than max, return #chars printed */
  register const char *p = t;

  if (!p) return 0;
  if (s) while (*p && max--) *s++ = *p++;
  else while (*p && max--) p++;

  return p - t;
}
