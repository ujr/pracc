#include <assert.h>
#include <dirent.h>
#include <errno.h>
#define _GNU_SOURCE
#include <fnmatch.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#include "pracc.h"
#include "webgui.h"

#define OK (0)
#define FAIL (-1)
#define MAX(x,y) ((x)>(y)?(x):(y))

static char **accts_array = 0;
static unsigned long accts_length = 0; // entries
static unsigned long accts_size = 0; // capacity
static long firstindex, lastindex;

static int acctcomp(const void *p1, const void *p2)
{
   return strcmp(*(char * const *) p1, *(char * const *) p2);
}

/*
 * Build a sorted array of all accounts (pracc files).
 * Do not yet get info about each account because this
 * would take too much time!
 *
 * Return 0 if ok, -1 on error (with errno set).
 */
int accts_init(long first, long count, const char *filter)
{
   DIR *dir;
   struct dirent *d;
   int i = 0;
   char *s;

   firstindex = MAX(first,1);
   lastindex = first + MAX(count,0) - 1;

   if (accts_array) {
      free((void *) accts_array);
      accts_array = 0;
      accts_length = accts_size = 0;
   }

   dir = opendir(PRACCDIR);
   if (!dir) return FAIL;

   while ((d = readdir(dir))) {
      if (strcmp(d->d_name, ".") == 0) continue;
      if (strcmp(d->d_name, "..") == 0) continue;
      if (praccCheckName(d->d_name) != 0) continue;
      if (filter && fnmatch(filter, d->d_name, FNM_CASEFOLD)) continue;

      if (i == accts_size) {
         size_t n = (i) ? i+i : 10;
         void *p = realloc(accts_array, n * sizeof(char **));
         if (!p) return FAIL;
//fprintf(stderr, "accts_init: realloc'ed %d elements\n", n);//DEBUG
         accts_size = n;
         accts_array = p;
      }

      if (!(s = strdup(d->d_name))) {
         closedir(dir);
         return FAIL;
      }
      else accts_array[i++] = s;
   }
   closedir(dir);

   accts_length = i;
//fprintf(stderr, "accts: sorting %ld accounts\n", accts_length);
   qsort(accts_array, accts_length, sizeof(char **), acctcomp);

   return OK;
}

/*
 * Return the values of account with the given index.
 * Index is 1..N, where N = accts_count().
 *
 * Return 0 if ok, 1 if no such index,
 * and -1 on any other error (with errno set).
 */
int accts_get(int index, char **acctname)
{
   if (index < 1) { errno = EINVAL; return FAIL; }
   if (index > accts_length) return 1; // no more entries

   if (index < firstindex) return 1;
   if (index > lastindex) return 1;

   if (acctname) *acctname = accts_array[index-1];

   return OK;
}

#if 0
int accts_get2(int index,
              char *c, char **n, char **g, long *b, long *l, time_t *t)
{
   char *acct;
   char class;
   time_t lastused;
   long balance, limit;
   static char gecos[64]; // XXX

   if ((index < 0) || (index >= accts_length)) return 1;

   acct = accts_array[index];
   class = praccAccountInfo(acct, gecos, sizeof(gecos));
   if (!class) return -1; // somebody took away our account?

   if (praccSum(acct, &balance, &limit, 0, 0, &lastused))
      return -1; // somebody took away our account?

   if (c) *c = class;
   if (n) *n = acct;
   if (g) *g = gecos;

   if (b) *b = balance;
   if (l) *l = limit;
   if (t) *t = lastused;

   return 0;
}
#endif

/*
 * Return the number of matching accounts.
 * Return -1 if unknown.
 */
long accts_count(void)
{
   return (accts_array) ? accts_length : -1;
}

/*
 * Dump all entries selected by the given filter
 * to the given FILE stream.
 *
 * Return 0 if ok, -1 on error (with errno set).
 */
int accts_dump(FILE *out, const char *filter)
{
   int r, i;

   assert(out);

   r = accts_init(1, LONG_MAX, filter);
   if (r < 0) return FAIL;

   for (i = 0; i < accts_length; i++) {
      char class, *acct;
      char gecos[64]; // XXX
      long balance, limit;
      time_t last;

      acct = accts_array[i];
      class = praccAccountInfo(acct, gecos, sizeof(gecos));
      if (class && !praccSum(acct, &balance, &limit, 0, 0, &last)) {
         char tstr[32], lstr[16];
         struct tm *tmp;

         tmp = localtime(&last);
         if (tmp) strftime(tstr, sizeof(tstr), "%Y-%m-%d %H%M", tmp);
         else snprintf(tstr, sizeof(tstr), "YYYY-mm-dd HHMM");

         if (limit == UNLIMITED) strcpy(lstr, "none");
         else snprintf(lstr, sizeof(lstr), "%ld", limit);

         fprintf(out, "%c\t%s\t%ld\t%s\t%s\t%s\n", class, acct,
                 balance, lstr, tstr, gecos);
      }
      else fprintf(out, "-\t%s\n", acct); // account vanished?!
   }

   return OK;
}

#ifdef TESTING

#include <errno.h>

const char *me;

void usage(const char *s);

int main(int argc, char **argv)
{
   const char *filter;
   int r;

   me = progname(argv);
   if (!me) return 127;

   ++argv;
   if (*argv) filter = *argv++;
   else filter = (const char *) 0;
   if (*argv) usage("too many arguments");

   r = accts_dump(stdout, filter);
   if (r < 0) {
      fprintf(stderr, "%s: reading accounts failed: %s\n",
              me, strerror(errno));
      return 111;
   }

   return 0; // OK
}

void usage(const char *s)
{
   if (s) fprintf(stderr, "%s: %s\n", me, s);
   fprintf(stderr, "Usage: %s [filter]\n", me);
   fprintf(stderr, "Filter is a shell wildcard pattern\n");
   exit(127);
}

#endif // TESTING
