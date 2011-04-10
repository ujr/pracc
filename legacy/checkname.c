#include "pracc.h"

void checkname(const char *acctname)
{ /* die if invalid account name */
  register const char *p = acctname;

  while (*p) if (!isgraph(*p++)) usage("invalid account name");
  if (p - acctname > MAXNAME) usage("account name too long");
}
