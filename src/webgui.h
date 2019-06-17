
#include <stdio.h>
#include <time.h>

/* accounts */

int accts_init(long first, long count, const char *filter);
long accts_count(void);
int accts_get(int index, char **acctname);
int accts_dump(FILE *out, const char *filter);

/* account */

int acct_init(long first, long count, const char *acctname,
              time_t tmin, time_t tmax, const char *types);

long acct_count(void);

int acct_get(int index, time_t *t, char *type,
             long *value, char **username, char **comment);

int acct_dump(FILE *out, const char *acctname,
              time_t tmin, time_t tmax, const char *types);

/* pclog */

int pclog_init(long first, long count, time_t tmin, time_t tmax,
               const char *filter);
long pclog_count(void);
void pclog_totals(long *pages, long *jobs);
int pclog_get(int index, char **name, time_t *t,
              long *pc, long *pages, long *jobs);
int pclog_dump(FILE *out, time_t tmin, time_t tmax, const char *filter);

/* pracclog */

int pracclog_init(long first, long count, time_t tmin, time_t tmax,
                  const char *filter, const char *types);
long pracclog_count(void);
int pracclog_get(int index, time_t *t, char **userp,
                        char **acctp, char **infop);
int pracclog_dump(FILE *out, time_t tmin, time_t tmax,
                  const char *filter, const char *types);

/* report */

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

