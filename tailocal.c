#include <time.h>

#include "tai.h"

time_t tailocal(struct tai *taip, struct tm *tmp)
{ /* convert a TAI label to Unix time and local time */
  struct tm *tp;
  time_t t;

  if (!taip) return (time_t) -1;

  t = (time_t) taiunix(taip);
  tp = localtime(&t); /* XXX */
  if (tmp) *tmp = *tp;
  return t; /* Unix time */
}
