/* Analyze the pracc pagecount log file */

#include <assert.h>
#include <errno.h>
#include <fnmatch.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "getln.h"
#include "pracc.h"
#include "symtab.h"
#include "tai.h"
#include "ui_pclog.h"

#define OK (0)
#define FAIL (-1)

static struct symbol *pclog_array = 0;
static unsigned long pclog_length = 0;

static void copysym(struct symbol *sym)
{
   static int i = 0;

   if (sym) pclog_array[i++] = *sym;
   else i = 0; // reset index counter
}

static int symcomp(const void *p1, const void *p2)
{
   const char *s1 = ((struct symbol *) p1)->name;
   const char *s2 = ((struct symbol *) p2)->name;

   return strcmp(s1, s2);
}

/*
 * Parse the pc.log file and compute the pages printed
 * per printer matched by the given filter and within
 * the given time period. The result is stored in the
 * global array pclog_array of size pclog_length.
 *
 * Note: first and count are ignored.
 *
 * Return 0 if ok, -1 on error (with errno set).
 */
int pclog_init(long first, long count, time_t tmin, time_t tmax,
               const char *filter)
{
   FILE *logfp;
   char line[MAXLINE];
   long int pc;
   char *printer;
   int i, n, skipped;
   struct symtab hash;
   char str[32];

   /* Empty filter means no filter = match all */
   if (filter && (filter[0] == '\0')) filter = 0;

   if (pclog_array) {
      copysym(0); // reset index counter
      for (i = 0; i < pclog_length; i++) {
         free((void *) pclog_array[i].name);
         free((void *) pclog_array[i].sval);
      }
      free(pclog_array);
      pclog_length = 0;
   }

   logfp = fopen(PRACCPCLOG, "r");
   if (!logfp) return FAIL; // see errno

   syminit(&hash, 128);

   while ((n = getln(logfp, line, sizeof(line), &skipped)) > 0) {
      char *p = line;
      struct tai tai;
      time_t tstamp;
      struct symbol *sym;

      /* Parse one line: @taistamp pc printer */

      if (n = taiscan(p, &tai)) p += n;
      else continue; // skip bad line
      tstamp = taiunix(&tai);

      while (isspace(*p)) ++p;

      if (n = scani(p, &pc)) p += n;
      else continue; // skip bad line

      while (isspace(*p)) ++p;

      printer = p;
      while (*p && !isspace(*p)) ++p;
      *p = '\0';

//fprintf(stderr, "pclog_init(): Parsed line: %ld %s\n", pc, printer);//DEBUG

      /* Filter by date and name */

      if (filter && filter[0] && fnmatch(filter, printer, 0)) continue;
      if ((tmin >= 0) && (tstamp < tmin)) continue; // too early
      if ((tmax >= 0) && (tmax < tstamp)) continue; // too late

      /* Update per-printer values */

      sym = symget(&hash, printer);
      if (sym) {
         sym->i0++; // counter
         if (pc >= 0) {
            if (pc < sym->i1 || sym->i1 < 0) sym->i1 = pc; // min
            if (sym->i2 < pc) sym->i2 = pc; // max
         }
         sprintf((char *) sym->sval, "%08lx",
                 (unsigned long) tstamp); // last seen
      }
      else {
         char *p = strdup(printer);
         if (!p) return FAIL;
//fprintf(stderr, "pclog_init(): First seen printer %s\n", p);//DEBUG
         sym = symput(&hash, p);
         if (!sym) return FAIL;
         sym->i0 = 1; // counter
         sym->i1 = sym->i2 = pc; // min=max
         sym->sval = malloc(8+1); // xxxxxxxx\0
         if (!sym->sval) return FAIL;
         sprintf((char *) sym->sval, "%08lx", (unsigned long) tstamp);
      }
   }
   if (n < 0) return FAIL;

   /* Make sorted array of pointers to symbols */

   pclog_length = symcount(&hash);
   pclog_array = calloc(pclog_length, sizeof(struct symbol));
   if (!pclog_array) return FAIL;
   symeach(&hash, copysym);
   qsort(pclog_array, pclog_length, sizeof(struct symbol), symcomp);

   symkill(&hash);

   return OK;
}

/*
 * Return the number of matching printers.
 * Return -1 if unknown.
 */
long pclog_count(void)
{
   return (pclog_array) ? pclog_length : -1;
}

/*
 * Count total number of pages and print jobs
 * for the tmin/tmax and filter passed to init.
 */
void pclog_totals(long *pagesp, long *jobsp)
{
   int i;
   long pages, jobs;
   long count;

   pages = jobs = 0;
   count = pclog_count();

   for (i = 0; i < count; i++) {
      long pc1 = pclog_array[i].i1;
      long pc2 = pclog_array[i].i2;
      if ((pc1 >= 0) && (pc2 > pc1))
         pages += pc2 - pc1;
      jobs += pclog_array[i].i0;
   }

   if (pagesp) *pagesp = pages;
   if (jobsp) *jobsp = jobs;
}

/*
 * Copy values for given index into variables.
 *
 * Return 0 if ok, 1 if no such index,
 * and -1 on any other error (with errno set).
 */
int pclog_get(int index, char **name, time_t *tp,
              long *pc, long *pages, long *jobs)
{
   if (index < 1) { errno = EINVAL; return FAIL; }
   if (index > pclog_length) return 1; // no more entries

   index--; // from 1-based to 0-based indices
   if (name) *name = (char *) pclog_array[index].name;
   if (tp) sscanf(pclog_array[index].sval, "%lx", tp);
   if (pc) *pc = pclog_array[index].i2;
   if (pages) {
      long pc1 = pclog_array[index].i1;
      long pc2 = pclog_array[index].i2;
      if ((pc1 >= 0) && (pc2 >= pc1))
         *pages = pc2 - pc1;
      else *pages = -1; // n/a
   }
   if (jobs) *jobs = pclog_array[index].i0;

   return OK;
}

/*
 * Dump all printers selected by the given filter
 * to the given FILE stream.
 *
 * Return 0 if ok, -1 on error (with errno set).
 */
int pclog_dump(FILE *out, time_t tmin, time_t tmax, const char *filter)
{
   int r, i;

   assert(out);

   r = pclog_init(1, LONG_MAX, tmin, tmax, filter);
   if (r < 0) return FAIL;

   for (i = 0; i < pclog_length; i++) {
      time_t t;
      struct tm *tmp;
      char buf[64];

      long jc = pclog_array[i].i0;
      long p1 = pclog_array[i].i1;
      long p2 = pclog_array[i].i2;
      long pp = ((p1<0) || (p2<p1)) ? -1 : p2-p1;

      sscanf(pclog_array[i].sval, "%lx", &t);
      tmp = localtime(&t);
      if (tmp) strftime(buf, sizeof(buf), "%Y-%m-%d %H%M", tmp);
      else snprintf(buf, sizeof(buf), "YYYY-mm-dd HHMM");

      fprintf(out, "%s\t%ld\t%ld\t%ld\t%s\n",
         pclog_array[i].name, p2, pp, jc, buf);
   }

   return OK;
}

/* Roughly the same in AWK:
 *
 * { ct[$3]++;
 *   if (!mi[$3]) mi[$3]=$2
 *   else if (mi[$3]>$2) mi[$3]=$2
 *   if (!ma[$3]) ma[$3]=$2
 *   else if (ma[$3]<$2) ma[$3]=$2 }
 * END {
 *   for (pp in ct)
 *     print pp "\t" mi[pp] " " ma[pp] "\tpages " (ma[pp]-mi[pp]) \
 *       "\tcount " ct[pp]
 * }
 */

#ifdef TESTING

#include <errno.h>
#include <time.h>
#include <unistd.h>

#include "common.h"

const char *me;

void setdate(const char *s, time_t *tp);
void usage(const char *s);

int main(int argc, char **argv)
{
   time_t tmin, tmax;
   const char *filter;
   int c, i, r;

   extern int opterr;
   extern int optind;

   me = progname(argv);
   if (!me) return 127;

   opterr = 0; // prevent stupid getopt output
   tmin = tmax = -1;
   while ((c = getopt(argc, argv, "f:u:V")) > 0) switch (c) {
      case 'f': setdate(optarg, &tmin); break;
      case 'u': setdate(optarg, &tmax); break;
      case 'V':
         printf("This is pracc-pclog, version %s\n", VERSION);
         return 0;
      default: usage("invalid option");
   }
   argc -= optind;
   argv += optind;

   if (*argv) filter = *argv++;
   else filter = (const char *) 0;

   if (*argv) usage("too many arguments");

   r = pclog_dump(stdout, tmin, tmax, filter);
   if (r < 0) {
      fprintf(stderr, "%s: reading %s failed: %s\n",
              me, PRACCLOG, strerror(errno));
      return 111;
   }

   return 0; // OK
}

void setdate(const char *s, time_t *tp)
{
   struct tm tm;
   time_t t;
   int n;

   if ((n = scandate(s, &tm)) == 0)
      usage("invalid date argument");

   tm.tm_sec = tm.tm_min = tm.tm_hour = 0;
   if ((t = mktime(&tm)) < 0)
      usage("invalid date argument");

   if (tp) *tp = t;
}

void usage(const char *s)
{
   if (s) fprintf(stderr, "%s: %s\n", me, s);
   fprintf(stderr, "Usage: %s [-V] [-f date] [-u date] [filter]\n", me);
   fprintf(stderr, "Dates are in format yyyy-mm-dd\n");
   fprintf(stderr, "Filter is a shell wildcard pattern\n");
   exit(127);
}
#endif /* TESTING */
