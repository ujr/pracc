/* pracc-ok - sum up pracc file and see if ok to print
 *
 * Usage: pracc-ok [-V] [limit]
 *
 * Read a pracc file from stdin and sum up all credit, debit,
 * and reset lines.  Write the current credits to stdout; the
 * exit value indicates how the current credits relate to the
 * specified limit (default 0):
 *
 *      0  ok, credits > limit
 *      1  credits <= limit
 *    111  temporary error, e.g. troubles reading or writing
 *    255  generic error
 *
 * Option -V identifies program and version and exits 0.
 *
 * ujr/2003-08-14, ujr/2003-09-11
 *
 * ujr/2005-11-18
 * Rewritten to work with both pracc v1 and v2 files.
 * Invocation, exit values, and output is exactly the same,
 * for it is used by the filter whose source is lost...
 * TO BE USED ONLY AS LONG AS THE OLD FILTER IS IN USE!
 */

static char id[] = "pracc-ok by ujr/2005-11-18\n";

#define UNLIMITED 9999999

#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>

int getline(char *buf, int max, int stop);

int scani(const char *s, signed long *v);
int scanu(const char *s, unsigned long *v);
int itype(char *s, signed long i);
int utype(char *s, unsigned long u);

char *progname = "pracc-ok";
long value, credit=0, limit=0;
char c, *p, buf[32];

int main(int argc, char **argv)
{
  if (*argv && **argv) progname = *argv++;
  if (*argv && strcmp(*argv, "-V") == 0)
  	write(1, id, (sizeof id) - 1), exit(0);
  if (*argv && **argv) scani(*argv, &limit);

  while (getline(p=buf, sizeof buf, '\n') > 0) {
  	switch (c = *p++) {
  		case '-':  /* debit line */
  			scanu(p, &value);
  			credit -= value;
  			break;
  		case '+':  /* credit line */
  			scanu(p, &value);
  			credit += value;
  			break;
  		case '=':  /* reset line; might be negative! */
  			scani(p, &credit);
  			break;
  		case '$':  /* limit line - OVERRIDES COMMAND LINE */
  			if (c=scani(p, &value)) limit = value;
  			else limit = UNLIMITED; /* unlimited */
  			break;
  		default:  /* skip */
  			break;
  	}
  }
  write(1, "credit ", 7);
  write(1, buf, itype(buf, credit));
  write(1, "\n", 1);
  if (limit >= UNLIMITED) return 0;
  return (credit > limit) ? 0 : 1;
}

/* getline: get line into buf, return length */
int getline(char *buf, int max, int stop)
{
  int c, i=0;

  while (--max > 0 && (c=getchar()) != stop && c != EOF)
  	buf[i++] = c;
  if (c == stop) buf[i++] = c;
  else if (c == EOF) return EOF;
  else { /* drop rest of line */
  	do c = getchar();
  	while (c != stop && c != EOF);
  	buf[i++] = stop; }
  buf[i] = '\0';  /* termination */
  return i;
}

/* scanu: scan unsigned decimal value, return #chars scanned */
int scanu(const char *s, unsigned long *v)
{
  unsigned long c, val = 0;
  int i = 0;

  while ((c = (unsigned long) (s[i++] - '0')) < 10)
  	val = val * 10 + c;
  *v = val;

  return i-1;  /* num of chars scanned */
}

/* scani: scan signed decimal value, return #chars scanned */
int scani(const char *s, signed long *v)
{
  int sign, i = 0;

  sign = (*s == '-'); if (sign) s++;
  i = scanu(s, (unsigned long *) v);
  if (sign) *v *= -1;
  if (sign) i++;
  return i;  /* num of chars scanned */
}
 
/* utype: type an unsigned long, return #chars typed */
int utype(char *s, unsigned long u)
{
  register unsigned long v = u;
  register int n = 1;

  while (v > 9) { v/=10; n++; }  /* find #digits */
  if (s) { s += n; /* start at end of buffer */
  	do { *--s = '0' + (u % 10); u/=10; }
  	while (u > 0);
  }
  return n;  /* num of chars typed */
}

/* itype: type a signed long, return #chars typed */
int itype(char *s, long i)
{
  register int sign = (i < 0);

  if (sign) { *s++ = '-'; i *= -1; }
  return utype(s, (unsigned long) i) + sign;
}
