/* pracc-init.c - a utility in the pracc package
 * Copyright (c) 2005-2008 by Urs Jakob Ruetschi
 */

#include <errno.h>
#include <pwd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "pracc.h"
#include "print.h"
#include "putln.h"
#include "scan.h"

#define streq(s,t) (strcmp((s),(t)) == 0)

void setbalance(const char *s);
void setlimit(const char *s);
void usage(const char *s);

char *me;
int overwrite;
int nevermind;
char *acctname;
long balance;
long limit;

int main(int argc, char **argv)
{
   struct passwd *pw;
   const char *who;
   char comment[MAXLINE];
   const char *endp;
   char *p;
   int c;

   extern int optind;
   extern int opterr;

   me = progname(argv);
   if (!me) return 127; // no arg0

   opterr = 0; // prevent stupid getopt output
   overwrite = nevermind = 0; // defaults
   while ((c = getopt(argc, argv, "fFhV")) > 0) switch (c) {
      case 'f': overwrite = 1; break;
      case 'F': nevermind = 1; break;
      case 'h': usage(0); break; // show help
      case 'V': return praccIdentify("pracc-init");
      default:  usage("invalid option");
   }
   argc -= optind;
   argv += optind;

   if (*argv) acctname = *argv++;
   else usage("account not specified");
   if (praccCheckName(acctname))
      usage("invalid account name");

   if (*argv) setbalance(*argv++);
   else usage("balance not specified");

   if (*argv) setlimit(*argv++);
   else usage("limit not specified");

   if ((pw = getpwuid(getuid())) == 0)
      die(111, "getpwuid failed");
   who = pw->pw_name;

   p = comment;
   endp = comment + sizeof(comment) - 1;
   while (*argv) { // remaining args
      unsigned len = strlen(*argv);
      if (p+1+len > endp) break;
      if (p > comment) p += printc(p, ' ');
      p += prints(p, *argv++);
   }
   p += printc(p, '\0');

   umask(0007); // no rights for world
   if (praccCreate(acctname, balance, limit, who, comment, overwrite) < 0) {
      if (nevermind && (errno == EEXIST)) return 0;
      die(111, "cannot create account %s", acctname);
   }

/*
 * Build and append a line to pracc.log
 */

   p = comment;
   p += prints(p, "init ");
   p += printi(p, balance);
   p += prints(p, " limit ");
   if (limit <= UNLIMITED)
      p += printc(p, '*');
   else p += printi(p, limit);
   if (overwrite)
      p += prints(p, " overwrite");
   p += printc(p, '\0');

   if (praccLogup(who, acctname, comment) < 0)
      putfmt(stderr, "%s: cannot append to %s: %s\n",
             me, PRACCLOG, strerror(errno));

   return 0; // SUCCESS
}

void setbalance(const char *s)
{
   int n = scanu(s, (unsigned long *) &balance); // non-negative
   if (n == 0) usage("invalid balance");
}

void setlimit(const char *s)
{
   long value;
   int n = scani(s, &value);
   if (n > 0) limit = value;
   else if (streq(s, "none")) limit = UNLIMITED;
   else usage("invalid limit");
}

void usage(const char *err)
{
   FILE *fp = (err) ? stderr : stdout;
   if (err) putfmt(stderr, "%s: %s\n", me, err);
   putfmt(fp, "Usage: %s [-fFV] account balance limit {info}\n", me);
   putfmt(fp, "Create initial pracc file with given balance and limit\n");
   exit((err) ? 127 : 0); // FAILURE or OK
}
