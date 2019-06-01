#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"

/*
 * Terminate program execution with the given status
 * code, after printing an error message to stderr.
 */
void die(int code, const char *fmt, ...)
{
   extern char *me;
   char buf[1024], *p = buf;
   char *end = buf + sizeof(buf);

   va_list ap;
   va_start(ap, fmt);

   p += snprintf(p, end-p, "%s: ", me);
   p += vsnprintf(p, end-p, fmt, ap);
   if (errno) // system error
      p += snprintf(p, end-p, ": %s", strerror(errno));

   va_end(ap);

   if (p < end) *p++ = '\n';
   else *(p-1) = '\n';
   putbuf(stderr, buf, p-buf);

   exit(code);
}

