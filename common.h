/* Common includes for many pracc tools */

#include <stdio.h>

int putln(FILE *fp, const char *s);
int putfmt(FILE *fp, const char *fmt, ...);
int putbuf(FILE *fp, const char *buf, unsigned len);

extern char *progname(char **argv);

extern void die(int code, const char *fmt, ...);
