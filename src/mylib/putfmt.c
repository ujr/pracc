#include <stdarg.h>
#include <stdio.h>

#include "putln.h"

int putfmt(FILE *fp, const char *fmt, ...)
{
   int status;
   va_list ap;

   va_start(ap, fmt);
   status = vfprintf(fp, fmt, ap);
   va_end(ap);

   return (status < 0) ? -1 : 0;
}

