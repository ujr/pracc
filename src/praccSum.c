#include "pracc.h"

/*
 * Scan through the pracc file for the given account
 * and sum up its balance determine the current limit.
 * The tmin and tmax parameters define a time range
 * over which to sum up; if set to -1 they mean minus
 * or plus infinity, respectively.
 *
 * Return 0 if successful, -1 on error (see errno).
 */
int praccSumRange(const char *acctname, time_t tmin, time_t tmax,
   long *balp, long *limp, long *credp, long *debp, time_t *lastp)
{
   struct praccbuf pracc;
   long balance, limit, n;
   long credits, debits;
   time_t lastused = 0;

   if (praccOpen(acctname, &pracc) < 0) return -1;

   balance = 0;
   limit = UNLIMITED;
   credits = debits = 0;

   while ((n = praccRead(&pracc)) > 0) {
      long value = pracc.value;
      time_t t = pracc.tstamp;

      switch (pracc.type) {
         case '-': // debit
            balance -= value;
            break;
         case '+': // credit
            balance += value;
            break;
         case '=': // reset
            balance = value;
            break;
         case '$': // limit
            limit = value;
            break;
         case '#': // note
            continue; // skip - it has neither timestamp nor amount
         default: // skip all other types
            continue;
      }

      /* Stop processing beyond end date, if set */
      if ((tmax >= 0) && (tmax < t)) break;

      /* Update totals only within specified period */
      if ((tmin < 0) || (tmin <= t)) switch (pracc.type) {
         case '-': debits += pracc.value; break;
         case '+': credits += pracc.value; break;
      }

      /* Update timestamp of last record */
      lastused = t;
   }

   if (balp) *balp = balance;
   if (limp) *limp = limit;
   if (debp) *debp = debits;
   if (credp) *credp = credits;
   if (lastp) *lastp = lastused;

   (void) praccClose(&pracc);

   return (n < 0) ? -1 : 0;
}

/* Convenience: sum up over all times... */
int praccSum(const char *acctname,
   long *balp, long *limp, long *credp, long *debp, time_t *lastp)
{
   return praccSumRange(acctname, -1, -1, balp, limp, credp, debp, lastp);
}
