#ifndef _UI_REPORT_H_
#define _UI_REPORT_H_

#include <stdio.h>

struct acctinfo {
   const char *acct;
   long balance;
   long limit;
   long credits;
   long debits;
   time_t lastused;
};

int report_init(long first, long count, time_t tmin, time_t tmax,
                const char *acctlist);
long report_count(void);
int report_get(int index, struct acctinfo **aip);
void report_totals(long *credits, long *debits);
int report_dump(FILE *out, time_t tmin, time_t tmax, const char *acctlist);

#endif // _UI_REPORT_H_
