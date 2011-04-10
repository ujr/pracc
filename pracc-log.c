/* pracc-log.c - a utility in the pracc package
 * $Id: pracc-log.c,v 1.6 2008/02/06 21:48:54 ujr Exp ujr $
 * Copyright (c) 2006-2008 by Urs Jakob Ruetschi
 */
static char id[] = "This is pracc-log by ujr\n$Revision: 1.6 $\n";

/* TODO: get TZ in mktime and localtime right! */

#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "common.h"
#include "pracc.h"
#include "scan.h"

char *me;
char *acctname;
time_t datemin = 0;
time_t datemax = 0;

void setdate(const char *s, time_t *t);
void usage(const char *s);

int main(int argc, char **argv)
{
   struct pracclog pl;
   char buf[MAXLINE];
   struct tm *tmp;
   int c, n;

   extern int optind;
   extern char *optarg;
   extern int opterr;

   me = progname(argv);
   if (!me) return 127; // no arg0

   opterr = 0; // prevent stupid getopt output
   while ((c = getopt(argc, argv, "f:u:V")) > 0) switch (c) {
      case 'f': setdate(optarg, &datemin); break;
      case 'u': setdate(optarg, &datemax); datemax += 86400; break;
      case 'V': return (putln(stdout, id) == 0) ? 0 : 127;
      default: usage("invalid option");
   }
   argc -= optind;
   argv += optind;

   if (*argv) { // account name (optional)
      acctname = *argv++;
      if (praccCheckName(acctname))
         usage("invalid account name");
   }
   else acctname = 0;

   if (*argv) usage("too many arguments");

   if (praccLogOpen(&pl) < 0)
      die(111, "Cannot open %s", PRACCLOG);

   while ((n = praccLogRead(&pl)) > 0) {
      if (pl.tstamp < datemin) continue;
      if ((pl.tstamp > datemax) && (datemax > datemin)) continue;
      if (acctname && strcmp(acctname, pl.acctname)) continue;

      tmp = localtime(&pl.tstamp);
      if (tmp) putbuf(stdout, buf, printstm(buf, tmp));
      else putfmt(stdout, "%s", "yyyy-mm-dd HH:MM:SS");
      putfmt(stdout, " %s %s ", pl.username, pl.acctname);
      putln(stdout, pl.infostr);
   }
   praccLogClose(&pl);
   if (n < 0)
      die(111, "error reading %s", PRACCLOG);

   return 0; // SUCCESS
}

void setdate(const char *s, time_t *tp)
{
   register const char *p;
   struct tm tm;
   int n;

   n = scandate(s, &tm);
   if (n == 0) usage("invalid date argument");
   tm.tm_sec = tm.tm_min = tm.tm_hour = 0;
   *tp = mktime(&tm);
   if (*tp < 0) usage("invalid date argument");
}

void usage(const char *s)
{
   if (s) putfmt(stderr, "%s: %s\n", me, s);
   putfmt(stderr,
      "Usage: %s [-V] [-f from] [-u until] [account]\n", me);
   putln(stderr, " if account specified, look for pertinent log entries");
   putln(stderr, " from, until: date in ISO 8601 format, eg, 2005-07-15");
   exit(127); // FAILURE
}
