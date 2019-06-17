#ifndef _PUTLN_H_
#define _PUTLN_H_

#include <stdio.h>

int putln(FILE *fp, const char *s);
int putbuf(FILE *fp, const char *buf, unsigned len);
int putfmt(FILE *fp, const char *fmt, ...);

#endif /* _PUTLN_H_ */
