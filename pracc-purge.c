/* pracc-purge.c - a utility in the pracc package
 * Copyright (c) 2006-2008 by Urs Jakob Ruetschi
 */

#include <errno.h>
#include <pwd.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "common.h"
#include "pracc.h"
#include "print.h"
#include "scan.h"

#define SUFFIX ".purge"

int printdate(char *s, time_t *tp);
void setdate(const char *s, time_t *tp);
void usage(const char *s);

char *me;
char *acctname;

int main(int argc, char **argv)
{
   char *fn, *fntmp;
   time_t breaktime;
   int rflag, lflag, nflag, simul;
   struct passwd *pw;
   const char *who;
   int c, len;

   extern int optind;
   extern char *optarg;
   extern int opterr;

   me = progname(argv);
   if (!me) return 127; // no arg0

   opterr = 0; // prevent stupid getopt output
   rflag = lflag = nflag = simul = 0; // defaults
   while ((c = getopt(argc, argv, "lrnDhV")) > 0) switch (c) {
      case 'l': lflag = 1; break;
      case 'r': rflag = 1; break;
      case 'n': nflag = 1; break;
      case 'D': simul = 1; break;
      case 'h': usage(0); break; // show help
      case 'V': return praccIdentify("pracc-purge");
      default:  usage("invalid option");
   }
   argc -= optind;
   argv += optind;

   if (*argv) acctname = *argv++;
   else usage("no account specified");
   if (praccCheckName(acctname))
      usage("invalid account name");

   if (*argv) setdate(*argv++, &breaktime);
   else usage("no date specified");

   if (*argv) usage("too many arguments");

   if ((fn = praccPath(acctname)) == 0)
      die(111, "praccPath(%s) failed", acctname);

   if ((pw = getpwuid(getuid())) == 0)
      die(111, "getpwuid %s failed", getuid());
   who = pw->pw_name;

   len = strlen(fn) + strlen(SUFFIX) + 1;
   if ((fntmp = calloc(len, sizeof(char))) == 0)
      die(111, "calloc for %d chars failed", len);
   strcpy(fntmp, fn);
   strcat(fntmp, SUFFIX);

   if (praccPurge(acctname, breaktime, rflag, lflag, nflag, !simul, fntmp) < 0)
      die(111, "purging account %s failed", acctname);

   if (!simul) {
      char comment[MAXLINE];
      register char *p;

      p = comment;
      p += prints(p, "purge older than ");
      p += printdate(p, &breaktime);
      p += printc(p, '\0');

      if (praccLogup(who, acctname, comment) < 0)
         putfmt(stderr, "%s: cannot append to %s: %s\n",
                me, PRACCLOG, strerror(errno));
   }

   return 0; // SUCCESS
}
   
int printdate(char *s, time_t *tp)
{
   struct tm *tm;
   register char *p;

   tm = localtime(tp);
   if (!tm) return 0;

   p = s;
   p += printu(p, 1900 + tm->tm_year);
   p += printc(p, '-');
   if (tm->tm_mon < 9)
      p += printc(p, '0');
   p += printu(p, 1 + tm->tm_mon);
   p += printc(p, '-');
   if (tm->tm_mday < 10)
      p += printc(p, '0');
   p += printu(p, tm->tm_mday);

   return p - s; // #chars printed
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

void usage(const char *err)
{
   FILE *fp = (err) ? stderr : stdout;
   if (err) putfmt(stderr, "%s: %s\n", me, err);
   putfmt(fp, "Usage: %s [-lrnDV] account date\n", me);
   putfmt(fp, "Purge records older than <date> from <account>.\n");
   putfmt(fp, "Specify the date as yyyy-mm-dd, e.g., 2005-12-31\n");
   exit((err) ? 127 : 0); // FAILURE or OK
}
