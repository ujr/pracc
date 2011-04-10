/* Convert old timestamps in pracc files to TAI stamps */
/* Write to standard output */

#include "common.h"
#include "pracc.h"

char *me;
char *acctname;

int main(int argc, char **argv)
{
   struct praccbuf pracc;
   char s[64];
   int n;

   me = progname(argv);
   if (!me) return 127;

   acctname = *++argv;
   if (!acctname || *++argv) {
      fprintf(stderr, "Usage: convert <acctname>\n");
      return 127;
   }

   if (praccOpen(acctname, &pracc) < 0) {
      perror(acctname);
      return 111;
   }

   while ((n = praccRead(&pracc)) > 0) {
      switch (pracc.type) {
      case '#':
         printf("# %s\n", pracc.comment);
         break;
      case '!':
         s[taifmt(s, &pracc.taistamp)] = '\0';
         printf("! %s %s %s\n", s, pracc.username, pracc.comment);
         break;
      default:
         s[taifmt(s, &pracc.taistamp)] = '\0';
         printf("%c%ld %s %s %s\n", pracc.type, pracc.value,
                s, pracc.username, pracc.comment);
      }
   }
   if (n < 0) {
      perror(acctname);
      return 113;
   }
   return 0;
}

