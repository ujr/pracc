/* pracc-view.c - a utility in the pracc package
 * Copyright (c) 2005-2008 by Urs Jakob Ruetschi
 */

/* TODO: get TZ in mktime and localtime right! */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "common.h"
#include "pracc.h"
#include "scan.h"

#define OUT_OF_MEMORY (NULL)
#define streq(s,t) (strcmp((s),(t)) == 0)

void addtype(const char *s);
void setdate(const char *s, time_t *t);
void usage(const char *s);

char *me;
char *acctname;
time_t datemin = 0;
time_t datemax = 0;
char *linetypes = 0;

int main(int argc, char **argv)
{
   struct praccbuf pracc;
   char buf[256];
   int c, n;

   extern int optind;
   extern char *optarg;
   extern int opterr;

   me = progname(argv);
   if (!me) return 127; // no arg0

   opterr = 0; // prevent stupid getopt output
   while ((c = getopt(argc, argv, "f:t:u:hV")) > 0) switch (c) {
      case 't': addtype(optarg); break;
      case 'f': setdate(optarg, &datemin); break;
      case 'u': setdate(optarg, &datemax); datemax += 86400; break;
      case 'h': usage(0); // show help
      case 'V': return praccIdentify("pracc-view");
      default:  usage("invalid option");
   }
   argc -= optind;
   argv += optind;

   if (*argv) acctname = *argv++;
   else usage("account not specified");
   if (praccCheckName(acctname))
      usage("invalid account name");

   if (*argv) usage("too many arguments");

   if (praccOpen(acctname, &pracc) < 0)
      die(111, "cannot open %s", pracc.fn ? pracc.fn : acctname);

   while ((n = praccRead(&pracc)) > 0) {
      struct tm *tmp;
      time_t t = pracc.tstamp;

      if (t < datemin) continue;
      if (t > datemax && datemax > datemin) continue;
      if (pracc.type == '#') continue; // skip comments
      if (linetypes && !strchr(linetypes, pracc.type)) continue;

      tmp = localtime(&t);
      if (tmp) putbuf(stdout, buf, printstm(buf, tmp));
      else putfmt(stdout, "%s", "yyyy-mm-dd HH:MM:SS");

      putfmt(stdout, " %s %s ", pracc.username, praccTypeString(pracc.type));
      if (pracc.type != '!') {
         if ((pracc.type == '$') && (pracc.value == UNLIMITED))
            putfmt(stdout, "none ");
         else putfmt(stdout, "%d ", pracc.value);
      }
      putln(stdout, pracc.comment);
   }
   (void) praccClose(&pracc);

   return (n < 0) ? 111 : 0;
}

void addtype(const char *s)
{ /* add type s to list of types to show */
   if (!linetypes) {
      linetypes = strdup("......");
      assert(linetypes != OUT_OF_MEMORY);
   }

   if (streq(s, "debit")) linetypes[0] = '-';
   else if (streq(s, "credit")) linetypes[1] = '+';
   else if (streq(s, "reset")) linetypes[2] = '=';
   else if (streq(s, "limit")) linetypes[3] = '$';
   else if (streq(s, "error")) linetypes[4] = '!';
   else if (streq(s, "note")) linetypes[5] = '#';
   else usage("invalid argument to -t option");
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
   putfmt(fp, "Usage: %s [-V] [-f from] [-u until] {-t type} account\n",me);
   putfmt(fp, "List named account to standard output.\n");
   putfmt(fp, " known types: debit, credit, reset, limit, error, note\n");
   putfmt(fp, " from, until: date in yyyy-mm-dd format, eg, 2005-07-15\n");
   exit((err) ? 127 : 0); // FAILURE or OK
}
