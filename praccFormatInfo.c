#include "pracc.h"

#include <ctype.h>

/*
 * Copy at most size chars from info to buf, stop at the
 * first naughty char encountered in info. Translate tabs
 * to blanks while copying (they're naughty but we're nice).
 */
int praccFormatInfo(char *buf, const char *info, int size)
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

