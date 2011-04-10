#include "pracc.h"

#include <ctype.h>
#include <errno.h>

/*
 * Check if a given string is a valid pracc account name.
 * Pracc account names consist of at most MAXNAME printable
 * characters, excluding blanks, slashes, and backslashes.
 *
 * Return 0 if valid, -1 if invalid.
 */
int praccCheckName(const char *acctname)
{
   register const char *p;

   errno = 0; // not a system error
   if (!acctname || !acctname[0]) return -1;
   for (p = acctname; *p; p++) {
      if (!isprint(*p)) return -1;
      if (isblank(*p)) return -1;
      if (*p == '/') return -1;
      if (*p == '\\') return -1;
   }
   if (p - acctname > MAXNAME) return -1; // too long

   return 0; // acctname valid
}

