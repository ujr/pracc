/* pracc-scan.c - a utility in the pracc package
 * Copyright (c) 2012 by Urs-Jakob Ruetschi
 */

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "common.h"
#include "config.h"
#include "joblex.h"
#include "papersize.h"
#include "pracc.h"
#include "printer.h"

#define OK (0)
#define OUT_OF_MEMORY (NULL)

// Work in progress...
#pragma GCC diagnostic ignored "-Wunused-function"

static void usage(const char *err);
static int ceiling(float f);
static int configure(const char *key, const char *value, void *data);
static char flagchar(int flag);

const char *me;
int verbose = 0;   // 0=silent, 1=info, 2=debug, 3=more debug, etc.
int printer_found = 0;

int
main(int argc, char **argv)
{
   FILE *fp;
   int c, pages, duplex, color;
   const char *paper;

   const char *printername = 0;
   struct printer printer;

   extern int optind;
   extern char *optarg;
   extern int opterr;

   verbose = 1; // info
   me = progname(argv);
   if (!me) return 127; // no arg0

   opterr = 0; // prevent getopt output
   while ((c = getopt(argc, argv, "hp:qvV")) > 0) switch (c) {
      case 'h': usage(0); return OK; // show help
      case 'p': printername = optarg; break;
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

   printer_init(&printer, printername);

   config_parse_file(PRACCCONFIG, configure, &printer);

   if (printer.name && !printer_found) {
      fatal("%s: no such printer configured", printer.name);
      return 127;
   }

   debug("Printer: %s", printer.name ? printer.name : "(default)");
   debug(" canDuplex: %s", printer.can_duplex ? "yes" : "no");
   debug(" canColor: %s", printer.can_color ? "yes" : "no");
   debug(" canCopies: %s", printer.can_copies ? "yes" : "no");
   papersize_lookup_area(printer.default_pagesize, &paper, 0, 0);
   debug(" defaultPaper: %s (%.0f)", paper, printer.default_pagesize);

   joblex(fp, &printer, verbose);

   printer_report(&printer, &pages, &duplex, &color, &paper);

   //                 ?/0/1     ?/0/1  A4/A3/.. nup/dup/booklet UJU6U
   printf("pages=%d color=%c duplex=%c paper=%s layout=? struct=%s\n",
          pages, flagchar(color), flagchar(duplex),
          paper, printer.structure);

   return OK;
}

static char
flagchar(int flag)
{
   if (flag < 0) return '?';
   if (flag == 0) return '0';
   return '1';
}

static int
configure(const char *key, const char *value, void *data)
{
   struct printer *printer = (struct printer *) data;
   const char *name = printer->name;
   float width, height;
   int index = 0;

   if (name) {
      index = config_match_sect(key, "printer", name);
      if (index > 0) printer_found = 1;
   }

   if (!index) {
      // No -p printer arg or printer not found:
      index = config_match_sect(key, "printer", NULL);
   }

   if (index > 0) {
      if (config_match_name(key+index, "canDuplex")) {
         int flag = config_get_bool(key, value);
         printer_can_duplex(printer, flag);
      }
      else if (config_match_name(key+index, "canColor")) {
         int flag = config_get_bool(key, value);
         printer_can_color(printer, flag);
      }
      else if (config_match_name(key+index, "defaultPaper")) {
         if (papersize_lookup_name(value, &width, &height)) {
            printer_default_pagesize(printer, width*height);
         }
         else {
            config_error("unknown paper size: %s", value);
            exit(127);
         }
//         const char *paper = strdup(value); // must copy!
//         assert(paper != OUT_OF_MEMORY);
//         printer_default_paper(printer, paper);
      }
   }

   return 0;
}

void // required by config.h
config_error(const char *fmt, ...)
{
   va_list ap;
   va_start(ap, fmt);
   fprintf(stderr, "%s: ", me);
   vfprintf(stderr, fmt, ap);
   fprintf(stderr, "\n");
   va_end(ap);
   exit(127);
}

static int
ceiling(float f)
{
   // Not exactly the math ceil(), but what
   // we need to compute is the sheets printed.
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
   fprintf(fp, "Read standard input if no file is specified.\n");
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

void // required by config.h
error(const char *fmt, ...)
{
   va_list ap;

   va_start(ap, fmt);
   vfprintf(stderr, fmt, ap);
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
