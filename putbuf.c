#include <stdio.h>

int putbuf(FILE *fp, const char *buf, unsigned len)
{
   size_t n = fwrite(buf, len, 1, fp);
   return (ferror(fp)) ? -1 : 0;
}

