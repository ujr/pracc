/* pracc-debit.c - a utility in the pracc package
 * $Id: pracc-debit.c,v 1.2 2007/12/14 20:30:24 ujr Exp ujr $
 * Copyright (c) 2005-2008 by Urs Jakob Ruetschi
 */
static char id[] = "This is pracc-debit by ujr\n$Revision: 1.2 $\n";

#include <errno.h>
#include <pwd.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "common.h"
#include "pracc.h"
#include "print.h"
#include "scan.h"

char *me;
char *account;
long amount;
struct passwd *pw;

void setamount(const char *s);

int main(int argc, char **argv)
{
   char buf[MAXLINE];
   char *p, *bufend;
   int c, praccfd;

   extern int optind;
   extern int opterr;

   me = progname(argv);
   if (!me) return 127; // no arg0

   opterr = 0; // prevent stupid getopt output
   while ((c = getopt(argc, argv, "V")) > 0) switch (c) {
      case 'V': return (putln(stdout, id) == 0) ? 0 : 127;
      default: usage("invalid option");
   }
   argc -= optind;
   argv += optind;

   if (*argv) account = *argv++;
   else usage("account not specified");

   if (*argv) setamount(*argv++);
   else usage("amount not specified");

   /* Validate account name */
   if (praccname(account))
      usage("invalid account name");

   /* Check access permission */
   if ((pw = getpwuid(getuid())) == 0)
      die(127, "getpwuid failed");
   switch (praccaccess(pw->pw_name, account)) {
      case 0: /* granted */ break;
      case 1: /* denied */
         errno = 0;
         die(127, "account %s: access denied", account);
      default: /* see errno */
         die(127, "account %s error", account);
   }

   /* Gather remaining arguments for the info field */
   p = buf;
   bufend = buf + sizeof(buf) - 1; // reserve one byte for \0
   while (*argv) { // remaining args
      unsigned len = strlen(*argv);
      if (p+len > bufend) break;
      if (p > buf) p += printc(p, ' ');
      p += prints(p, *argv++);
   }
   p += print0(p);

   /* Append the debit record in just one write */
   if (praccwrite(account, '-', amount, pw->pw_name, buf) < 0)
      die(111, "cannot debit account %s", account);

   return 0; // SUCESS
}

void setamount(const char *s)
{
   int n = scanu(s, (unsigned long *) &amount); // non-negative
   if (n == 0) usage("invalid amount");
}

void usage(const char *s)
{
   if (s) putfmt(stderr, "%s: %s\n", me, s);
   putfmt(stderr, "Usage: %s [-V] account amount {info}\n", me);
   exit(127); // FAILURE
}
