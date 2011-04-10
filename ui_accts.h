#ifndef _UI_ACCTS_H_
#define _UI_ACCTS_H_

#include <stdio.h>
#include <time.h>

extern int accts_init(long first, long count, const char *filter);
extern long accts_count(void);
extern int accts_get(int index, char **acctname);
extern int accts_dump(FILE *out, const char *filter);

#endif // _UI_ACCTS_H_
