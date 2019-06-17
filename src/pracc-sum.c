/* pracc-sum.c - a utility in the pracc package
 * Copyright (c) 2005-2008 by Urs Jakob Ruetschi
 */

/* TODO: get TZ in mktime and localtime right! */

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "pracc.h"
#include "print.h"
#include "putln.h"
#include "scan.h"

void setdate(const char *s, time_t *t);
void usage(const char *s);

char *me;
char *acctname;
time_t tmin = -1;
time_t tmax = -1;

int main(int argc, char **argv)
{
   long balance, limit;
   long credits, debits;
   time_t lastused;
   char limitstr[16];
   char laststr[32];
   int c, status;

   extern int optind;
   extern char *optarg;
   extern int opterr;

   me = progname(argv);
   if (!me) return 127; // no arg0

   opterr = 0; // prevent stupid getopt output
   while ((c = getopt(argc, argv, "f:u:hV")) > 0) switch (c) {
      case 'f': setdate(optarg, &tmin);
                break;
      case 'u': setdate(optarg, &tmax);
                tmax += 86399; // 23:59:59
                break;
      case 'h': usage(0); break; // show help
      case 'V': return praccIdentify("pracc-sum");
      default:  usage("invalid option");
   }
   argc -= optind;
   argv += optind;

   if (*argv) acctname = *argv++;
   else usage("account not specified");
   if (praccCheckName(acctname))
      usage("invalid account name");

   if (*argv) usage("too many arguments");

   balance = 0;
   limit = UNLIMITED;
   credits = debits = 0;

   if (praccSumRange(acctname, tmin, tmax, &balance, &limit,
                     &credits, &debits, &lastused) == 0) {
      struct tm *tmp;

      tmp = localtime(&lastused);
      if (tmp) sprintf(laststr, "%d-%02d-%02d",
              tmp->tm_year+1900, tmp->tm_mon+1, tmp->tm_mday);
      else sprintf(laststr, "%s", "yyyy-mm-dd HH:MM:SS");

      if (limit == UNLIMITED) strcpy(limitstr, "none");
      else limitstr[printi(limitstr, limit)] = '\0';

      putfmt(stdout,
             "acct %s balance %d limit %s credits %d debits %d last %s\n",
             acctname, balance, limitstr, credits, debits, laststr);
      status = (balance <= limit) && (limit != UNLIMITED) ? 1 : 0;
   }
   else {
      putfmt(stderr, "acct %s: %s\n", acctname, strerror(errno));
      status = 111;
   }

   return status;
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
   putfmt(fp, "Usage: %s [-V] [-f date] [-u date] account\n", me);
   putfmt(fp, "Compute the balance of the named account.\n");
   putfmt(fp, " date: in yyyy-mm-dd format, eg, 2005-07-15\n");
   exit((err) ? 127 : 0); // FAILURE or OK
}
