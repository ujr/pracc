#include "tai.h"

/* TAISTAMP: @4000000042d77da2 */

int taifmt(char s[1+TAIBYTES+TAIBYTES], struct tai *tp)
{
   static char hex[16] = "0123456789abcdef";

   char buf[TAIBYTES];
   int i;

   taistore(buf, tp);

   s[0] = '@';
   for (i=0; i<8; i++) {
      s[i*2+1] = hex[(buf[i] >> 4) & 15];
      s[i*2+2] = hex[(buf[i] & 15)];
   }
   return 1+16; // #bytes written to s
}
