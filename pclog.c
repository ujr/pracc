/* Analyze the pracc pagecount log file (pc.log) */

#include <assert.h>
#include <ctype.h>
#include <fnmatch.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "getln.h"
#include "pclog.h"
#include "pracc.h"
#include "scan.h"
#include "symtab.h"
#include "tai.h"

#define OK (0)
#define FAIL (-1)
#define OUT_OF_MEMORY (NULL)

static void
symcopy(struct symbol *sym, void *data)
{
   static int i = 0; // eek

   if (sym) {
      struct pclog *pc = (struct pclog *) data;
      if (i < pc->count) pc->array[i++] = *sym;
   }
   else {
      i = 0; // reset index
   }
}

static int
symcomp(const void *p1, const void *p2)
{
   const char *s1 = ((struct symbol *) p1)->name;
   const char *s2 = ((struct symbol *) p2)->name;

   return strcmp(s1, s2);
}

int
pclog_load(struct pclog *pc, time_t tmin, time_t tmax, const char *filter)
{
   FILE *logfp;
   char line[MAXLINE];
   struct symtab hash;
   int n, skipped;

   // Empty filter means no filter, ie, match all:
   if (filter && (filter[0] == '\0')) filter = 0;

   logfp = fopen(PRACCPCLOG, "r");
   if (!logfp) return FAIL; // see errno

   syminit(&hash, 128);

   while ((n = getln(logfp, line, sizeof(line), &skipped)) > 0) {
      char *p = line;
      struct tai tai;
      time_t tstamp;
      long int pcount;
      char *printer;
      struct symbol *sym;

      // Parse one line: @taistamp pc printer

      if ((n = taiscan(p, &tai))) p += n;
      else continue; // skip bad line
      tstamp = taiunix(&tai);

      while (isspace(*p)) ++p;

      if ((n = scani(p, &pcount))) p += n;
      else continue; // skip bad line

      while (isspace(*p)) ++p;

      printer = p;
      while (*p && !isspace(*p)) ++p;
      *p = '\0';

//fprintf(stderr, "pclog_load(): Parsed line: %ld %s\n", pc, printer); //DEBUG

      // Filter by date and name

      if (filter && filter[0] && fnmatch(filter, printer, 0)) continue;
      if ((tmin >= 0) && (tstamp < tmin)) continue; // too early
      if ((tmax >= 0) && (tmax < tstamp)) continue; // too late

      // Update per-printer values

      sym = symget(&hash, printer);
      if (sym) {
         sym->i0 += 1; // counter
         if (pcount >= 0) {
            if (pcount < sym->i1 || sym->i1 < 0) sym->i1 = pcount; // min
            if (sym->i2 < pcount) sym->i2 = pcount; // max
         }
         sprintf((char *) sym->sval, "%08lx",
                 (unsigned long) tstamp); // last seen
      }
      else {
         char *p = strdup(printer);
         assert(p != OUT_OF_MEMORY);
//fprintf(stderr, "pclog_load(): First seen printer %s\n", printer);//DEBUG
         sym = symput(&hash, p);
         assert(sym != OUT_OF_MEMORY);
         sym->i0 = 1; // counter
         sym->i1 = sym->i2 = pcount; // min=max
         sym->sval = malloc(8+1); // "xxxxxxxx\0"
         assert(sym->sval != OUT_OF_MEMORY);
         sprintf((char *) sym->sval, "%08lx", (unsigned long) tstamp);
      }
   }

   if (n < 0) return FAIL;

   // Make sorted array of pointers to symbols:

   pc->count = symcount(&hash);
   pc->array = calloc(pc->count, sizeof(struct symbol));
   assert(pc->array != OUT_OF_MEMORY);
   symcopy(NULL, NULL); // reset index
   symeach(&hash, (void *) pc, symcopy);
   qsort(pc->array, pc->count, sizeof(struct symbol), symcomp);

   symkill(&hash);

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

void
pclog_free(struct pclog *pc)
{
   int i;

   assert(pc != NULL);

   for (i = 0; i < pc->count; i++) {
      free((void *) pc->array[i].name);
      free((void *) pc->array[i].sval);
   }

   free((void *) pc->array);

   pc->count = 0;
   pc->array = NULL;
}

int
pclog_dump(FILE *fp, time_t tmin, time_t tmax, const char *filter)
{
   struct pclog pc;
   int i, r = pclog_load(&pc, tmin, tmax, filter);

   if (r < 0) return r;

      fprintf(stdout, "#Printer\tCounter\tPages\tJobs\tLastUsed\n");

   for (i = 0; i < pc.count; i++) {
      time_t t;
      struct tm *tmp;
      char buf[64];

      long jc = pc.array[i].i0;
      long p1 = pc.array[i].i1;
      long p2 = pc.array[i].i2;
      long pp = ((p1<0) || (p2<p1)) ? -1 : p2-p1;

      sscanf(pc.array[i].sval, "%lx", &t);
      tmp = localtime(&t);
      if (tmp) strftime(buf, sizeof(buf), "%Y-%m-%d %H%M", tmp);
      else snprintf(buf, sizeof(buf), "YYYY-mm-dd HHMM");

      fprintf(stdout, "%s\t%ld\t%ld\t%ld\t%s\n",
         pc.array[i].name, p2, pp, jc, buf);
   }

   pclog_free(&pc);

   return OK;
}
