#ifndef _PRINT_H_
#define _PRINT_H_

/* The routines here render a string representation in the buffer
 * specified as the 1st argument from the object given in the 2nd
 * argument.  They return the number of chars in the representation,
 * even if the buffer is NULL (this is useful for determining lengths
 * beforehand).
 */
#define printc(s, c) ((s) ? *(s)=c, 1 : 1)  /* XXX */
#define print0(s) ((s) ? *(s)='\0', 1 : 1)  /* XXX */
unsigned prints(char *s, const char *t);
unsigned printsn(char *s, const char *t, unsigned max);
unsigned printu(char *s, unsigned long u);
unsigned print0u(char *s, unsigned long u, unsigned n);
unsigned printi(char *s, signed long i);
unsigned printx(char *s, unsigned long x);

#include <time.h>
int printstm(char *s, struct tm *tp);

#endif /* _PRINT_H_ */
