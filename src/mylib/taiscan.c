#include "tai.h"

/* Scan string s for a TAI timestamp */
int taiscan(const char *s, struct tai *tp)
{
   if (s && (*s == '@')) {
      register int i;
      register uint64 secs = 0;
      for (i = 1; i < 17; i++) { register int x = s[i];
         if (('0' <= x) && (x <= '9')) x -= '0'; /* ASCII */
         else if (('a' <= x) && (x <= 'f')) x += 10 - 'a'; /* ASCII */
         else if (('A' <= x) && (x <= 'F')) x += 10 - 'A'; /* ASCII */
         else return 0; //break;
         secs <<= 4;
         secs += x;
      }
      if (tp) tp->x = secs;
      return i; // #chars scanned
   }
   return 0; // nothing scanned
}
