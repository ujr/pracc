/* pracc-pclog.c - a utility in the pracc package
 * Copyright (c) 2006-2012 by Urs Jakob Ruetschi
 */

/* TODO: get TZ in mktime and localtime right! */

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "common.h"
#include "pclog.h"
#include "pracc.h"
#include "scan.h"

char *me;

void setdate(const char *s, time_t *t);
void usage(const char *s);

int main(int argc, char **argv)
{
   time_t tmin = -1;
   time_t tmax = -1;
   char *filter = 0;
   int c, r;

   extern int optind;
   extern char *optarg;
   extern int opterr;

   me = progname(argv);
   if (!me) return 127; // no arg0

   opterr = 0; // prevent stupid getopt output
   while ((c = getopt(argc, argv, "f:u:hV")) > 0) switch (c) {
      case 'f': setdate(optarg, &tmin); break;
      case 'u': setdate(optarg, &tmax); tmax += 86400; break;
      case 'h': usage(0); break; // show help
      case 'V': return praccIdentify("pracc-log");
      default:  usage("invalid option");
   }
   argc -= optind;
   argv += optind;

   if (*argv) { // printer name filter (shell glob, optional)
      filter = *argv++;
   }

   if (*argv) usage("too many arguments");

   r = pclog_dump(stdout, tmin, tmax, filter);
   if (r < 0) {
      fprintf(stderr, "%s: reading %s failed: %s\n",
              me, PRACCPCLOG, strerror(errno));
      return 111;
   }

   return 0; // SUCCESS
}

void setdate(const char *s, time_t *tp)
{
   struct tm tm;
   int n;

   n = scandate(s, &tm);
   if (n == 0) usage("invalid date argument");
   tm.tm_sec = tm.tm_min = tm.tm_hour = 0;
   *tp = mktime(&tm);
   if (*tp < 0) usage("invalid date argument");
}

void usage(const char *err)
{
   FILE *fp = (err) ? stderr : stdout;
   if (err) putfmt(stderr, "%s: %s\n", me, err);
   putfmt(fp, "Usage: %s [-hV] [-f from] [-u until] [printer]\n", me);
   putln(fp, "Analyse pc.log entries to standard output.");
   putln(fp, " printer: analyse this printer (wildcards)");
   putln(fp, " from, until: date in yyyy-mm-dd format, eg, 2005-07-15");
   exit((err) ? 127 : 0); // FAILURE or OK
}
