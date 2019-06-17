#include "print.h"

int printstm(char *s, struct tm *tp)
{ /* print tp into s, return #bytes printed */
  register char *p = s;
  int n = printu(p, 1900+tp->tm_year);
  if (p) p += n; else return n+1+2+1+2+1+2+1+2+1+2;
  p += printc(p, '-');

  if (tp->tm_mon < 9) p += printc(p, '0');
  p += printu(p, 1 + tp->tm_mon);

  p += printc(p, '-');

  if (tp->tm_mday < 10) p += printc(p, '0');
  p += printu(p, tp->tm_mday);

  p += printc(p, ' ');

  if (tp->tm_hour < 10) p += printc(p, '0');
  p += printu(p, tp->tm_hour);

  p += printc(p, ':');

  if (tp->tm_min < 10) p += printc(p, '0');
  p += printu(p, tp->tm_min);

  p += printc(p, ':');

  if (tp->tm_sec < 10) p += printc(p, '0');
  p += printu(p, tp->tm_sec);

  return p - s; /* #chars printed */
}
