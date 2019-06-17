#include "scan.h"

int scanu(const char *s, unsigned long *v)
{ /* scan an unsigned decimal from s into v */
  unsigned long c, val = 0;
  int i = 0;

  if (s == 0) return 0;
  while ((c = (unsigned long) (s[i] - '0')) < 10) {
  	val = 10 * val + c;
  	i++;
  }
  if (v) *v = val;
  return i; /* num of chars scanned */
}
