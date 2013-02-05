#ifndef _PCL5_H_
#define _PCL5_H_

#include <stdio.h>

#include "printer.h"

extern int pcl5_parse(struct printer *prt, FILE *logfp, int verbose);

#endif // _PCL5_H_
