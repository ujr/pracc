#ifndef _DATETOOLS_H_
#define _DATETOOLS_H_

#include <time.h>

int parsedate(const char *s, time_t *tp);
int daterange(const char *period, time_t *tmin, time_t *tmax);

#endif // _DATETOOLS_H_
