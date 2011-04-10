#ifndef _UI_PRACCLOG_H_
#define _UI_PRACCLOG_H_

#include <stdio.h>
#include <time.h>

extern int pracclog_init(long first, long count, time_t tmin, time_t tmax,
                         const char *filter, const char *types);
extern long pracclog_count(void);
extern int pracclog_get(int index, time_t *t, char **userp,
                        char **acctp, char **infop);
extern int pracclog_dump(FILE *out, time_t tmin, time_t tmax,
                         const char *filter, const char *types);

#endif // _UI_PRACCLOG_H_
