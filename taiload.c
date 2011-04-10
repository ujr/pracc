#include "tai.h"

void taiload(char buf[TAIBYTES], struct tai *t)
{ /* load t from buf */
  register uint64 x = (unsigned char) buf[0];
  x <<= 8;  x += (unsigned char) buf[1];
  x <<= 8;  x += (unsigned char) buf[2];
  x <<= 8;  x += (unsigned char) buf[3];
  x <<= 8;  x += (unsigned char) buf[4];
  x <<= 8;  x += (unsigned char) buf[5];
  x <<= 8;  x += (unsigned char) buf[6];
  x <<= 8;  x += (unsigned char) buf[7];
  t->x = x;
}
