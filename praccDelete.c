#include "pracc.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

/*
 * Delete the given account by removing its pracc file.
 *
 * Return 0 if ok, -1 on error (with errno set).
 */
int praccDelete(const char *acctname)
{
   const char *fn;
   int r;

   assert(acctname);

   fn = praccPath(acctname);
   if (!fn) return -1; // ENOMEM

   r = remove(fn);

   free((void *) fn);

   return (r == 0) ? 0 : -1;
}
