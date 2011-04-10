#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "tai.h"

const char *me;

static void usage(const char *s)
{
   if (s) fprintf(stderr, "%s: %s\n", me, s);
   fprintf(stderr, "Usage: %s yyyy-mm-dd hh:mm:ss\n", me);
   exit(127);
}

int main(int argc, char **argv)
{
   char s[1024];

   if (argv && *argv) me = *argv++;
   else return 127; // no arg0?

   if (*argv) strcpy(s, *argv++); // XXX
   while (*argv) { strcat(s, " "); strcat(s, *argv++); } // XXX

   if (*s) {
      struct tai tai;
      if (oldscan(s, &tai)) {
         char buf[20];
         taifmt(buf, &tai);
         buf[17] = '\0';
         printf("%s\n", buf);
      }
      else usage("invalid date/time specified");
   }
   else usage("no date/time specified");

   return 0;
}
