#ifndef _TAISTAMP_H_
#define _TAISTAMP_H_

/* TAISTAMP: @xxxxxxxxxxxxxxxx */

#include <time.h>
#include "tai.h"

#define TAISTAMPLEN (1+TAIBYTES+TAIBYTES)

int taistamp(char s[1+TAIBYTES+TAIBYTES]);
//time_t tailocal(struct tai *taip, struct tm *tmp);

#endif /* _TAISTAMP_H_ */
