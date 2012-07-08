/* pracc-edit.c - part of pracc sources
 * Copyright (c) 2005-2008 by Urs Jakob Ruetschi
 */

#include <errno.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "common.h"
#include "pracc.h"
#include "print.h"
#include "scan.h"

#define streq(s,t) (strcmp((s),(t)) == 0)

void setaction(const char *s);
void setamount(const char *s);
void setbalance(const char *s);
void setlimit(const char *s);
void usage(const char *s);

char *me;
char *acctname;
char action; // one of: -, +, =, $, !, #
long number; // amount or balance or limit

int main(int argc, char **argv)
{
   struct passwd *pw;
   const char *who;
   char comment[MAXLINE];
   char *p, *endp;
   int c;

   extern int optind;
   extern int opterr;

   me = progname(argv);
   if (!me) return 127; // no arg0

   opterr = 0; // prevent stupid getopt output
   while ((c = getopt(argc, argv, "hV")) > 0) switch (c) {
      case 'h': usage(0); // show help
      case 'V': return praccIdentify("pracc-edit");
      default:  usage("invalid option");
   }
   argc -= optind;
   argv += optind;

   if (*argv) acctname = *argv++;
   else usage("account not specified");
   if (praccCheckName(acctname))
      usage("invalid account name");

   if (*argv) setaction(*argv++);
   else usage("action not specified");
   if (!praccTypeString(action))
      usage("invalid action");

   if ((pw = getpwuid(getuid())) == 0)
      die(111, "getpwuid(%d) failed", getuid());
   who = pw->pw_name;

   switch (action) {
      case '-': // debit
      case '+': // credit
         if (*argv) setamount(*argv++);
         else usage("amount not specified");
         break;
      case '=': // reset
         if (*argv) setbalance(*argv++);
         else usage("balance not specified");
         break;
      case '$': // limit
         if (*argv) setlimit(*argv++);
         else usage("limit not specified");
         break;
      case '!': // error
      case '#': // note
         break;
      default: usage("invalid action");
   }

   p = comment;
   endp = comment + sizeof(comment) - 1;
   while (*argv) { // remaining args
      unsigned len = strlen(*argv);
      if (p+len > endp) break;
      if (p > comment) *p++ = ' ';
      p += prints(p, *argv++);
   }
   *p = '\0'; // terminate

/*
 * Ok, here we have set from the cmdline args:
 *   action = debit / credit / reset / limit / note
 *   number = amount / balance / limit / 0 (if note)
 *   comment[] = text for info field, NUL-terminated
 * Now use the pracc API to append this record.
 */

   if (praccAppend(acctname, action, number, who, comment) < 0)
      die(111, "cannot write account %s", acctname);

/*
 * Build and append a line to pracc.log
 */

   p = comment;
   p += prints(p, praccTypeString(action));
   p += printc(p, ' ');
   switch (action) {
      case '-':
      case '+':
         p += printu(p, number); // amount
         break;
      case '=':
         p += printi(p, number); // balance
         break;
      case '$':
         if (number == UNLIMITED) *p++ = '*';
         else p += printi(p, number); // limit
         break;
      case '!':
      case '#':
         p += prints(p, "added");
         break;
   }
   *p = '\0'; // terminate

   if (praccLogup(who, acctname, comment) < 0)
      putfmt(stderr, "%s: cannot append to %s: %s\n",
             me, PRACCLOG, strerror(errno));

   return 0; // SUCCESS
}

void setaction(const char *s)
{
   if (streq(s, "debit")) action = '-';
   else if (streq(s, "credit")) action = '+';
   else if (streq(s, "reset")) action = '=';
   else if (streq(s, "limit")) action = '$';
   else if (streq(s, "error")) action = '!';
   else if (streq(s, "note")) action = '#';
   else usage("invalid action");
}

void setamount(const char *s)
{
   int n = scanu(s, (unsigned long *) &number);
   if (n == 0) usage("invalid amount");
}

void setbalance(const char *s)
{
   int n = scani(s, &number); // may be negative!
   if (n == 0) usage("invalid balance");
}

void setlimit(const char *s)
{
   long value;
   int n = scani(s, &value);
   if (n > 0) number = value;
   else if (streq(s, "none")) number = UNLIMITED;
   else usage("invalid limit");
}

void usage(const char *err)
{
   FILE *fp = (err) ? stderr : stdout;
   if (err) putfmt(stderr, "%s: %s\n", me, err);
   putfmt(fp, "Usage: %s [-V] account action {argument}\n", me);
   putfmt(fp, "Append a record to the named account.\n");
   putfmt(fp, " action: debit, credit, reset, limit, error, note\n");
   exit((err) ? 127 : 0); // FAILURE or OK
}
