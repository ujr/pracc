#include "tai.h"

void taistore(char buf[TAIBYTES], struct tai *t)
{ /* store t in buf */
  register uint64 x = t->x;
  buf[7] = x & 0xff;  x >>= 8;
  buf[6] = x & 0xff;  x >>= 8;
  buf[5] = x & 0xff;  x >>= 8;
  buf[4] = x & 0xff;  x >>= 8;
  buf[3] = x & 0xff;  x >>= 8;
  buf[2] = x & 0xff;  x >>= 8;
  buf[1] = x & 0xff;  x >>= 8;
  buf[0] = x;
}
