#ifndef _JOBLEX_H_
#define _JOBLEX_H_

#include <stdio.h>

struct printer {
   int copies, init_copies;
   int duplex, init_duplex; // -1=unknown, 0=simplex, 1=duplex
   int format, init_format; // -1=unknown, avg area of media
   int color;               // -1=unknown, 0=false, 1=true
   int pages;               // pages printed
   float sheets;            // sheets printed
   char structure[32];
};

extern void joblex(FILE *fp, struct printer *printer, int verbosity);

extern void debug(const char *fmt, ...);
// TODO debug(int lineno, const char *fmt, ...);

#endif // _JOBLEX_H_
