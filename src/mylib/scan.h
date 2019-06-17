#ifndef _SCAN_H_
#define _SCAN_H_

/*
 * All these scanners return the number of characters scanned.
 * They generally assume ASCII strings!
 */
extern int scans(const char *s, const char *t);          /* t at start of s */
extern int scani(const char *s, long *v);                /* signed dec */
extern int scanu(const char *s, unsigned long *v);       /* unsigned dec */
extern int scanx(const char *s, unsigned long *v);       /* hexadecimal */
extern int scanip4(const char *s, unsigned char ip[4]);  /* IPv4 addr */
extern int scanip4op(const char *s, unsigned char ip[4], unsigned short *port);

extern int scanpat(const char *s, const char *pat);      /* s starts with pat */
extern int scanuntil(const char *s, const char *stoppers, char *buf, int size);
extern int scanwhile(const char *s, const char *eaters, char *buf, int size);

#include <time.h> /* struct tm */
extern int scandate(const char *s, struct tm *tp);       /* 2005-07-15 */
extern int scantime(const char *s, struct tm *tp);       /* 12:34:45 */

#endif /* _SCAN_H_ */
