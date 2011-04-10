/* pracc-ok.c - a utility in the pracc package
 * $Id: pracc-ok.c,v 1.1 2008/02/06 21:49:06 ujr Exp ujr $
 * Copyright (c) 2005-2007 by Urs Jakob Ruetschi
 */

#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "pracc.h"

void usage(const char *s);

char *me;
char *acctname;

int main(int argc, char **argv)
{
   long balance, limit;
   int status;

   me = progname(argv++);
   if (!me) return 127; // no arg0

   if (*argv) acctname = *argv++;
   else usage("account not specified");
   if (praccCheckName(acctname))
      usage("invalid account name");

   if (*argv) usage("too many arguments");

   if (praccSum(acctname, &balance, &limit, 0, 0, 0) == 0) {
      char s[16];
      if (limit == UNLIMITED) strcpy(s, "none");
      else s[printi(s, limit)] = '\0';
      putfmt(stdout, "acct %s balance %d limit %s\n",
             acctname, balance, s);
      status = (balance <= limit) && (limit != UNLIMITED);
   }
   else {
      putfmt(stderr, "acct %s: %s\n", acctname, strerror(errno));
      status = 111;
   }

   return status;
}

void usage(const char *s)
{
   if (s) putfmt(stderr, "%s: %s\n", me, s);
   putfmt(stderr, "Usage: %s account\n", me);
   exit(127); // FAILURE
}
