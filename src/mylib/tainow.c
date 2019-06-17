#include "tai.h"

/* Note:
 * time(2) returns the number of seconds since "the epoch",
 * which is 1970-01-01 00:00:00 UTC and corresponds to TAISHIFT.
 */
void tainow(struct tai *now)
{ /* store current TAI64 label in now */
  now->x = TAISHIFT + (uint64) time((long) 0);
}
