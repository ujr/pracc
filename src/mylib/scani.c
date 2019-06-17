#include "scan.h"

int scani(const char *s, signed long *v)
{ /* scan a signed decimal from s into v */
  int sign, i = 0;

  if (s == 0) return 0;

  if (*s == '+') sign = 1;
  else if (*s == '-') sign = -1;
  else sign = 0;

  if (sign) ++s;
  i = scanu(s, (unsigned long *) v);
  if (i == 0) return 0;
  if (sign) { i += 1; if (v) *v *= sign; }

  return i; /* num of chars scanned */
}
