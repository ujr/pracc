#include <assert.h>
#include <errno.h>
#include <grp.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include "pracc.h"
#include "symtab.h"
#include "ui_report.h"

#define OK (0)
#define FAIL (-1)
#define DELIMS ", \t" // user/group delimiters
#define MAX(x,y) ((x)>(y)?(x):(y))

static struct acctinfo *replist = 0; // NULL if not allocated
static unsigned long replen = 0; // number of entries
static time_t mintime, maxtime;
static long total_credits, total_debits;
static long firstindex, lastindex;

static int comparator(const void *p1, const void *p2)
{
   struct acctinfo *a1 = (struct acctinfo *) p1;
   struct acctinfo *a2 = (struct acctinfo *) p2;

   return strcmp(a1->acct, a2->acct);
}

static int addname(struct symtab *syms, const char *name)
{
   assert(syms && name);

   if (!symget(syms, name)) {
      char *dups = strdup(name);
      if (!dups) return FAIL;
      if (!symput(syms, dups)) return FAIL;
   }
   return OK;
}

static void addacct(struct symbol *sym)
{
   long b, l, c, d;
   time_t t;

   if (sym && (praccSumRange(sym->name, mintime, maxtime,
                             &b, &l, &c, &d, &t) == 0)) {
      replist[replen].acct = sym->name;
      replist[replen].balance = b;
      replist[replen].limit = l;
      replist[replen].credits = c;
      replist[replen].debits = d;
      replist[replen].lastused = t;
      replen++;

      total_credits += c;
      total_debits += d;
   }
}

/*
 * Load pracc account names for the given list of users
 * into a sorted array for subsequent use. The users list
 * is a comma or space separated list of user and group
 * names, group names must be preceded by an @ sign.
 *
 * Return 0 if ok, -1 on error (with errno set).
 */
int report_init(long first, long count, time_t tmin, time_t tmax,
                const char *acctlist)
{
   char *s, *tok;
   struct group *gr;
   char **pp;
   struct symtab hash;
   struct symbol *sym;
   long i, size;

   firstindex = MAX(first,1);
   lastindex = first + MAX(count,0) - 1;

   if (replist) {
      for (i = 0; i < replen; i++)
         free((void *) replist[i].acct);
      free((void *) replist);
      replen = 0;
      replist = 0;
   }

   mintime = tmin;
   maxtime = tmax;

   /* No users/groups specified? */
   if (!acctlist || !acctlist[0])
      return OK;
   assert(acctlist && acctlist[0]);

   syminit(&hash, 128);

   s = strdup(acctlist);
   if (!s) return FAIL; // ENOMEM

   tok = strtok(s, DELIMS);
   while (tok) {
      if (tok[0] == '@') {
//fprintf(stderr, "group %s$\n", tok+1);//DEBUG
         gr = getgrnam(tok+1);
         if (gr) for (pp = gr->gr_mem; *pp; pp++) {
//fprintf(stderr, " %s\n", *pp);//DEBUG
            if (addname(&hash, *pp)) goto freefail;
         }
         else {
            if (!errno) errno = EINVAL; // no such group
            goto freefail;
         }
      }
      else {
//fprintf(stderr, "user %s$\n", tok);//DEBUG
         if (addname(&hash, tok)) goto freefail;
      }

      tok = strtok(NULL, DELIMS);
   }

   size = symcount(&hash);
   replist = calloc(size, sizeof(struct acctinfo));
   if (!replist) goto freefail;

   /* Gather info for each account and sum up */
   total_credits = total_debits = 0;
   symeach(&hash, addacct); // this defines replen

   /* Shrink list to actual size and sort */
   replist = realloc(replist, replen * sizeof(struct acctinfo));
   qsort(replist, replen, sizeof(struct acctinfo), comparator);

   symkill(&hash);
   free(s);

   return OK;

freefail:
   symkill(&hash);
   free(s); // release memory
   return FAIL; // see errno
}

/*
 * Return number of report entries.
 * Return -1 if this is not known.
 */
long report_count(void)
{
   return replen;
}

/* Valid only AFTER a call to report_init() */
void report_totals(long *credits, long *debits)
{
   if (credits) *credits = (replist) ? total_credits: -1;
   if (debits) *debits = (replist) ? total_debits : -1;
}

/*
 * Return report information for entry at given index.
 * Index is 1..N, where N = report_count().
 *
 * Return 0 if ok, 1 if no such index,
 * and -1 on any other error (with errno set).
 */
int report_get(int index, struct acctinfo **aip)
{
   if (index < 1) { errno = EINVAL; return FAIL; }
   if (index > replen) return 1; // no more entries

   if (index < firstindex) return 1;
   if (index > lastindex) return 1;

   if (aip) *aip = &replist[index-1];

   return OK;
}

int report_dump(FILE *out, time_t tmin, time_t tmax, const char *acctlist)
{
   int r, i, n;
   struct acctinfo *aip;

   assert(out);

   r = report_init(1, LONG_MAX, tmin, tmax, acctlist);
   if (r < 0) return FAIL; // see errno

   n = report_count();
   for (i = 1; i <= n; i++) {
      if (report_get(i, &aip) == 0) {
         char gecos[64], class;
         char tstr[32], lstr[16];
         struct tm *tmp;

         class = praccAccountInfo(aip->acct, gecos, sizeof(gecos));
         if (class) {
            tmp = localtime(&(aip->lastused));
            if (tmp) strftime(tstr, sizeof(tstr), "%Y-%m-%d %H%M", tmp);
            else snprintf(tstr, sizeof(tstr), "YYYY-mm-dd HHMM");

            if (aip->limit == UNLIMITED) strcpy(lstr, "none");
            else snprintf(lstr, sizeof(lstr), "%ld", aip->limit);

            fprintf(out, "%c\t%s\t%ld\t%s\t%ld\t%ld\t%s\t%s\n",
                    class, aip->acct, aip->balance, lstr,
                    aip->credits, aip->debits, tstr, gecos);
         }
      }
   }

   return OK;
}

#ifdef STANDALONE
int main(int argc, char **argv)
{
   int i;

   if (argv[0] && argv[1])
      if (report_init(-1, -1, argv[1]) < 0)
         printf("Error: %s\n", strerror(errno));

   report_dump(stdout, -1, -1, argv[1]);

   printf("Totals: %ld credits, %ld debits\n", total_credits, total_debits);

   return 0;
}
#endif
