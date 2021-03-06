#include <stdio.h>

#include "putln.h"

#define SEP '\n'

int putln(FILE *fp, const char *s)
{
   register char c = '\0';
   register const char *p = s;
   while (*p) putc(c = *p++, fp);
   if (c != SEP) putc(SEP, fp);
   return (ferror(fp)) ? -1 : 0;
}

