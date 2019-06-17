#include "print.h"

unsigned printi(char *s, signed long i)
{ /* print a signed long int, return #chars printed */
  register int sign = (i < 0);

  if (sign) { i *= -1; if (s) *s++ = '-'; }
  return printu(s, (unsigned long) i) + sign;
}
