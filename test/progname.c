#include "progname.h"

/*
 * Get the program name from the given arguments array,
 * which is the basename of the first argument, argv[0].
 *
 * Return pointer to progname or NULL if no such argument.
 */
char *progname(char **argv)
{
   const char *s = 0;
   register const char *p;

   if (argv && *argv) {
      p = s = *argv;
      while (*p) if (*p++ == '/')
         if (p) s = p; // advance
   }
   return (char *) s;
}

