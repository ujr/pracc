#include <assert.h>
#include <stdio.h>

#include "pracc.h"

/*
 * Write a line to stdout that identifies a pracc tool.
 * Return 0 if successful and 127 on failure (if we can't
 * write to stdout, this is considered a permanent error).
 */
int praccIdentify(const char *toolname)
{
   int retval;

   // Ensure #define is a string:
   const char *version = VERSION;

   assert(toolname != NULL);
   assert(version != NULL);

   retval = printf("This is %s, version %s\n", toolname, version);
   return (retval < 0) ? 127 : 0;
}
