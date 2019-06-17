#ifndef _PCLOG_H_
#define _PCLOG_H_

#include <stdio.h>
#include <time.h>
#include "symtab.h"

struct pclog {
   int count;
   struct symbol *array;
};

int pclog_load(struct pclog *pc, time_t tmin, time_t tmax, const char *filter);
void pclog_free(struct pclog *pc);

int pclog_dump(FILE *fp, time_t tmin, time_t tmax, const char *filter);

#endif // _PCLOG_H_
