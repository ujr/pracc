/* Analyze the pracc pagecount log file */

#include <assert.h>
#include <errno.h>
#include <fnmatch.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "getln.h"
#include "pclog.h"
#include "pracc.h"
#include "symtab.h"
#include "tai.h"
#include "ui_pclog.h"

#define OK (0)
#define FAIL (-1)

static struct pclog pc = {0, NULL};

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
   if (pc.array) pclog_free(&pc);
   return pclog_load(&pc, tmin, tmax, filter);
}

/*
 * Return the number of matching printers.
 * Return -1 if unknown.
 */
long pclog_count(void)
{
   return pc.array ? pc.count : -1;
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
      long pc1 = pc.array[i].i1;
      long pc2 = pc.array[i].i2;
      if ((pc1 >= 0) && (pc2 > pc1))
         pages += pc2 - pc1;
      jobs += pc.array[i].i0;
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
              long *pcount, long *pages, long *jobs)
{
   if (index < 1) { errno = EINVAL; return FAIL; }
   if (index > pc.count) return 1; // no more entries

   index--; // from 1-based to 0-based indices
   if (name) *name = (char *) pc.array[index].name;
   if (tp) sscanf(pc.array[index].sval, "%lx", tp);
   if (pcount) *pcount = pc.array[index].i2;
   if (pages) {
      long pc1 = pc.array[index].i1;
      long pc2 = pc.array[index].i2;
      if ((pc1 >= 0) && (pc2 >= pc1))
         *pages = pc2 - pc1;
      else *pages = -1; // n/a
   }
   if (jobs) *jobs = pc.array[index].i0;

   return OK;
}
