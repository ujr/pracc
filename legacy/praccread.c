#include <errno.h>
#include <stdio.h>
#include <string.h>

#include "pracc.h"
#include "scan.h"

/*
 * Read an account file to get its balance and limit.
 * Return 0 if ok, 1 if no pracc file, -1 on error.
 */

int praccread(const char *account, long *bp, long *lp)
{
   const char *fn;
   char line[16]; // only type and number needed
   int c, n;
   long value, balance, limit;
   unsigned long amount;
   FILE *fp;

   if ((fn = praccpath(account)) == 0)
      return -1; // ENOMEM
   if ((fp = fopen(fn, "r")) == 0) {
      int saverr = errno;
      free(fn);
      errno = saverr;
      return (errno == ENOENT) ? 1 : -1;
   }

   balance = limit = 0;
   while (fgets(line, sizeof(line), fp)) {
      char *p = line;
      int n = strlen(line);
      if (line[n-1] != '\n') { // skip
         do { c = getc(fp); }
         while ((c != '\n') && (c != EOF));
      }
      switch (c = *p++) {
      case '-': // debit line
         n = scanu(p, &amount);
         if (n > 0) balance -= amount;
         break;
      case '+': // credit line
         n = scanu(p, &amount);
         if (n > 0) balance += amount;
         break;
      case '=': // reset line
         n = scani(p, &value);
         if (n > 0) balance = value;
         break;
      case '$': // limit line
         n = scani(p, &value);
         if (n > 0) limit = value;
         else limit = UNLIMITED;
         break;
      default: // skip
         break;
      }
   }
   if (ferror(fp)) return -1;

   fclose(fp);
   free(fn);

   if (bp) *bp = balance;
   if (lp) *lp = limit;

   return 0; // SUCCESS
}
