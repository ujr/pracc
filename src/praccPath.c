#include "pracc.h"

#include <stdlib.h>
#include <string.h>

/*
 * Return a pointer to an malloc'ed string containing
 * the full path to the pracc file for the given acctname
 * or NULL on error.
 *
 * Note: This is the place to make changes if it is ever
 * decided that pracc files should be kept in a hashed
 * directory structure: /var/pracc/a/asmith, etc.
 */
char *praccPath(const char *acctname)
{
   int n = strlen(PRACCDIR);
   int m = strlen(acctname);

   char *path = (char *) calloc(n+1+m+1, sizeof(char));
   if (path == (char *) 0) return (char *) 0; // ENOMEM

   strcpy(path, PRACCDIR);
   if (path[n-1] != '/') path[n++] = '/';
   strcpy(path+n, acctname);

   return path;
}

