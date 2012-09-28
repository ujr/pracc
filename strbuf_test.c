/* Unit testing strbuf */

#include <assert.h>
#include <stdio.h>

#include "strbuf.h"

#define SIZEGTLEN(sp) ((sp)->size > (sp)->len)
#define TERMINATED(sp) (0 == (sp)->buf[(sp)->len])

int
main(int argc, char **argv)
{
   struct strbuf sb;
   struct strbuf *sp = &sb;
   int i;

   strbuf_init(sp, 0);
   assert(0 == sp->len);
   assert(SIZEGTLEN(sp));
   assert(TERMINATED(sp));
   
   strbuf_addb(sp, "gagax", 4);
   assert(strcmp(sp->buf, "gaga") == 0);
   assert('a' == strbuf_last(sp));
   assert(SIZEGTLEN(sp));
   assert(TERMINATED(sp));
   
   strbuf_addf(sp, "+%d-%d=%s", 3, 4, "konfus");
   assert(strcmp(sp->buf, "gaga+3-4=konfus") == 0);
   assert(SIZEGTLEN(sp));
   assert(TERMINATED(sp));
   fprintf(stderr, "sp->buf = [%s]\n", sp->buf);

   strbuf_clear(sp);
   assert(sp->len == 0);
   assert(0 == strbuf_length(sp));
   assert('\0' == strbuf_last(sp));
   assert(SIZEGTLEN(sp));
   assert(TERMINATED(sp));

   for (i = 0; i < 26; i++)
      strbuf_addc(sp, "abcdefghijklmnopqrstuvwxyz"[i]);
   for (i = 0; i < 26; i++)
      strbuf_addc(sp, "ABCDEFGHIJKLMNOPQRSTUVWXYZ"[i]);
   fprintf(stderr, "sp->buf = [%s]\n", sp->buf);
   assert(52 == strbuf_length(sp));
   assert('\0' == sp->buf[sp->len]);
   assert('Z' == strbuf_last(sp));
   assert(SIZEGTLEN(sp));
   assert(TERMINATED(sp));

   strbuf_setlen(sp, 26);
   assert(26 == strbuf_length(sp));
   assert(TERMINATED(sp));
   fprintf(stderr, "sp->buf = [%s]\n", sp->buf);

   printf("OK");

   return 0;
}
