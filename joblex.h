#ifndef _JOBLEX_H_
#define _JOBLEX_H_

#include <stdio.h>

#include "printer.h"

extern void joblex(FILE *fp, struct printer *printer, int verbosity);

extern void debug(const char *fmt, ...);
extern void fatal(const char *fmt, ...);
// TODO pass in a line number or byte offset (for better error reporting)

#endif // _JOBLEX_H_
