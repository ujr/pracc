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
static int ceiling(float f);
static void configure(const char *key, const char *value, void *data);

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

   joblex_printer_init(&printer);

   opterr = 0; // prevent getopt output
   while ((c = getopt(argc, argv, "hp:qvV")) > 0) switch (c) {
      case 'h': usage(0); return OK; // show help
      case 'p': printer.name = optarg; break;
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

   config_parse_file(PRACCCONFIG, configure, &printer);

   joblex(fp, &printer, verbose);

   //                          -1/0/1  A4/A3/.. nup/dup/booklet UJU6U
   printf("pages=%d sheets=%d color=%d paper=%s layout=%s struct=%s\n",
          printer.pages, ceiling(printer.sheets), printer.color,
          printer.init_paper, printer.layout, printer.structure);

   return OK;
}

static void
configure(const char *key, const char *value, void *data)
{
   struct printer *printer = (struct printer *) data;
   const char *name = printer->name;
   int index;

//fprintf(stderr, "%s=%s$\n", key, value);

   index = config_match_sect(key, "printer", name);
   if (index > 0) {
      if (config_match_name(key+index, "canDuplex")) {
         int flag = config_get_bool(key, value);
         joblex_printer_can_duplex(printer, flag);
      }
      else if (config_match_name(key+index, "canColor")) {
         int flag = config_get_bool(key, value);
         joblex_printer_can_color(printer, flag);
      }
   }
}

static int
ceiling(float f)
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

void // required by config.h
die(int code, const char *fmt, ...)
{
   va_list ap;
   va_start(ap, fmt);
   fprintf(stderr, "%s: ", me);
   vfprintf(stderr, fmt, ap);
   fprintf(stderr, "\n");
   va_end(ap);
   exit(code);
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
