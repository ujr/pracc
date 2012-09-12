/* pracc-scan.c - a utility in the pracc package
 * Copyright (c) 2012 by Urs-Jakob Ruetschi
 */

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "joblex.h"
#include "pracc.h"

#define OK (0)
//#define OUT_OF_MEMORY (NULL)

static void usage(const char *err);
static int ceil(float f);

const char *me;
int verbose = 0;   // 0=silent, 1=info, 2=debug
struct printer printer;

int
main(int argc, char **argv)
{
   FILE *fp;
   int c;

   extern int optind;
   extern char *optarg;
   extern int opterr;

   verbose = 0;
   me = progname(argv);
   if (!me) return 127; // no arg0

   opterr = 0; // prevent getopt output
   while ((c = getopt(argc, argv, "hqvV")) > 0) switch (c) {
      case 'h': usage(0); return OK; // show help
      case 'q': verbose = 0; break; // be quiet
      case 'v': verbose += 1; break; // more verbose
      case 'V': return praccIdentify("pracc-scan");
      default: usage("invalid option"); return 127;
   }
   argc -= optind;
   argv += optind;

   if (*argv) {
      const char *fn = *argv++;
      if (!(fp = fopen(fn, "r"))) {
         fprintf(stderr, "%s: open %s: %s\n", me, fn, strerror(errno));
         return 111;
      }
   }
   else {
      fp = stdin;
   }

   if (*argv) {
      usage("too many arguments");
      return 127;
   }

   joblex_printer_init(&printer);

   joblex(fp, &printer, verbose);

   // Really needed: pages=%d sheets=%d color=%d paper=%d nup=%d struct=%s\n
   //                                   -1/0/1   A4/A3/etc       UJU6U
   printf("%d pages=%d sheets=%d copies=%d duplex=%d color=%d struct=%s\n",
          printer.pages, printer.pages, ceil(printer.sheets),
          printer.copies, printer.duplex, printer.color, printer.structure);

   return OK;
}

static int
ceil(float f)
{
   // Not exactly the math ceil(), but what
   // we need to compute the sheets printed.
   if (f <= 0) return 0;
   int i = (int) f;
   if (i < f) i += 1;
   return i;
}

void
usage(const char *err)
{
   FILE *fp = err ? stderr : stdout;
   if (err) fprintf(stderr, "%s: %s\n", me, err);
   fprintf(fp, "Usage: %s [-V] [-h] [-qv] [file]\n", me);
   fprintf(fp, "Scan print job for page count, duplex, etc.\n");
   fprintf(fp, "Known job formats: PJL, PCL5, PCL6, PostScript.\n");
   fprintf(fp, "Options: -q quiet, -v increase verbosity, -V identify\n");
   fprintf(fp, "Read standard input if no jobfile is specified.\n");
   exit(err ? 127 : OK);
}

void // required by joblex.h
debug(const char *fmt, ...)
{
   va_list ap;

   if (verbose < 2) return;

   va_start(ap, fmt);
   vfprintf(stderr, fmt, ap);
   //if (lineno > 0) fprintf(stderr, " (line %ld)\n", lineno); else
   fprintf(stderr, "\n");
   va_end(ap);
}

void // required by joblex.h
fatal(const char *fmt, ...)
{
   va_list ap;

   va_start(ap, fmt);
   vfprintf(stderr, fmt, ap);
   fprintf(stderr, "\n");
   va_end(ap);
   exit(111); // give up => TODO report an estimate (pracc-scan must never fail)
}
