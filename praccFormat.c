
#include "pracc.h"

#include <ctype.h>

/**
 * Pracc does not handle usernames that contain spaces.
 * This routine simply copies the given user/acct name
 * up to the first space into the buffer provided.
 *
 * Truncated names are flagged with an exclamation (!) mark.
 * This routine is used by all API functions that write to
 * pracc files or the common log file.
 */
int
praccFormatName(const char *name, char *buf, int size)
{
   register int c, i = 0;

   if (size <= 0) return 0;
   if (size > MAXNAME) size = MAXNAME;
   while ((c = *name++) && (i < size)) {
      if (isspace(c)) break;
      else buf[i++] = c;
   }
   if (c) { // flag truncation!
      if (i < size) buf[i] = '!';
      else buf[size-1] = '!';
   }
   return i; // #chars written to buf
}

/**
 * Copy at most size chars from info to buf, stop at the
 * first naughty char encountered in info. Translate tabs
 * to blanks while copying (they're naughty but we're nice).
 */
int
praccFormatInfo(const char *info, char *buf, int size)
{
   register int c, i = 0;

   if (size <= 0) return 0;
   while ((c = *info++) && (i < size)) {
      if (c == '\t') c = ' '; // no tabs please
      if (isgraph(c) || (c == ' ')) buf[i++] = c;
      else break;
   }
   return i; // #chars written to buf
}

