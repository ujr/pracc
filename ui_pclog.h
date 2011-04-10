#ifndef _UI_PCLOG_H_
#define _UI_PCLOG_H_

#include "symtab.h"
#include "tai.h"

extern int pclog_init(long first, long count, time_t tmin, time_t tmax,
                      const char *filter);
extern long pclog_count(void);
extern void pclog_totals(long *pages, long *jobs);
extern int pclog_get(int index, char **name, time_t *t,
                     long *pc, long *pages, long *jobs);
int pclog_dump(FILE *out, time_t tmin, time_t tmax, const char *filter);

#endif // _UI_PCLOG_H_
