#ifndef _UI_ACCT_H_
#define _UI_ACCT_H_

#include <stdio.h>
#include <time.h>

extern int
acct_init(long first, long count, const char *acctname,
          time_t tmin, time_t tmax, const char *types);

extern long
acct_count(void);

extern int
acct_get(int index, time_t *t, char *type,
         long *value, char **username, char **comment);

extern int
acct_dump(FILE *out, const char *acctname,
          time_t tmin, time_t tmax, const char *types);

#endif // _UI_ACCT_H_
