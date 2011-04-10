#include "pracc.h"

/*
 * Pracc does not handle usernames that contain spaces.
 * This routine simply copies the given user name up to
 * the first space into the buffer provided. Truncated
 * names are flagged with an exclamation (!) mark. This
 * routine is used by all routines that write pracc files.
 */
int praccuser(char *buf, const char *user, int size)
{
   register int c, i = 0;

   if (size <= 0) return 0;
   if (size > MAXNAME) size = MAXNAME;
   while ((c = *user++) && (i < size)) {
      if (isspace(c)) break;
      else buf[i++] = c;
   }
   if (c) { // flag truncation!
      if (i < size) buf[i] = '!';
      else buf[size-1] = '!';
   }

   return i; // #chars written to buf
}
