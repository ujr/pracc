/* Utilities common to many pracc tools */

#include <stdio.h> // for FILE

void usage(const char *s); // just the prototype

char *progname(char **argv);
char *strtype(char type);

int skipline(FILE *fp);
int getln(FILE *fp, char *buf, int size);
int putln(FILE *fp, const char *s);
int putfmt(FILE *fp, const char *fmt, ...);
int putbuf(FILE *fp, const char *buf, int len);

/* Appending to the common log file */
int logup(const char *buf, int len);

/* Suicide */
void die(int code, const char *fmt, ...);
