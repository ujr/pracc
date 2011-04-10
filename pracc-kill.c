/* pracc-view.c - a utility in the pracc package
 * $Id: pracc-kill.c,v 1.1 2008/04/05 18:03:51 ujr Exp ujr $
 * Copyright (c) 2005-2008 by Urs Jakob Ruetschi
 */
static char id[] = "This is pracc-kill by ujr\n$Revision: 1.1 $\n";

#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "common.h"
#include "pracc.h"
#include "print.h"

void usage(const char *s);

char *me;
char *acctname;
int serious = 0;

int main(int argc, char **argv)
{
   struct passwd *pw;
   const char *who;
   long balance, limit;
   char s[16], *p;
   char buf[MAXLINE];
   const char *endp;
   int c;

   extern int optind;
   extern int opterr;

   me = progname(argv);
   if (!me) return 127; // no arg0

   opterr = 0; // prevent stupid getopt output
   while ((c = getopt(argc, argv, "fV")) > 0) switch (c) {
      case 'f': serious = 1; break;
      case 'V': return (putln(stdout, id) == 0) ? 0 : 127;
      default: usage("invalid option");
   }
   argc -= optind;
   argv += optind;

   if (*argv) acctname = *argv++;
   else usage("account not specified");
   if (praccCheckName(acctname))
      usage("invalid account name");

   if ((pw = getpwuid(getuid())) == 0)
      die(111, "getpwuid %s failed", getuid());
   who = pw->pw_name;

/*
 * Ok, here we have parsed command line arguments
 * into variables and are ready to delete the account.
 * Before deleting, we get the current balance and limit
 * for inclusion in the log entry.
 */

   if (praccSum(acctname, &balance, &limit, 0, 0, 0) < 0)
      die(111, "cannot sum up account %s", acctname);

   if (limit == UNLIMITED) strcpy(s, "none");
   else snprintf(s, sizeof(s), "%ld", limit);
   snprintf(buf, sizeof(buf), "delete balance=%ld limit=%s", balance, s);

   p = buf + strlen(buf);
   endp = buf + sizeof(buf) - 1;
   if (p < endp) {
      while (*argv) { // remaining arguments
         unsigned len = strlen(*argv);
         if (p+len > endp) break;
         p += printc(p, ' ');
         p += prints(p, *argv++);
      }
      *p = '\0';
   }

   if (serious) {
      if (praccDelete(acctname) < 0)
         die(111, "cannot delete account %s", acctname);

      if (praccLogup(who, acctname, buf) < 0)
         die(111, "cannot add log record");
   }
   else {
      printf("acct %s: %s\n", acctname, buf);
      printf("Account not deleted, use -f to really delete it.\n");
   }

   return 0; // SUCCESS
}

void usage(const char *s)
{
   if (s) putfmt(stderr, "%s: %s\n", me, s);
   putfmt(stderr, "Usage: %s [-V] [-f] account [comment]\n",me);
   putfmt(stderr, "Delete given account (if -f, otherwise just pretend).\n");
   putfmt(stderr, "Include the comment, if any, in the pracc log entry.\n");
   exit(127); // FAILURE
}
