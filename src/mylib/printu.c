#include "print.h"

unsigned printu(char *s, unsigned long u)
{ /* print an unsigned long, return #chars printed */
  register unsigned long v = u;
  register unsigned int n = 1;

  while (v > 9) { v/=10; n++; }  /* find #digits */
  if (s) { s += n; /* start at end of buffer */
        do { *--s = '0' + (u % 10); u/=10; }
        while (u > 0);
  }
  return n;  /* num of chars typed */
}
