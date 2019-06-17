#ifndef _GETLN_H_
#define _GETLN_H_

#include <stdio.h>

int getln(FILE *fp, char *buf, int size, int *skipped);
int eatln(FILE *fp);

#endif /* _GETLN_H_ */
