#include "taistamp.h"

/* UTCSTAMP: 2005-07-15 12:34:56Z */
/* TAISTAMP: @4000000042d77da2 */

int taistamp(char s[1+TAIBYTES+TAIBYTES])
{ /* store TAI timestamp in s */
  static char hex[16] = "0123456789abcdef";

  struct tai now;
  char buf[TAIBYTES];
  int i;

  tainow(&now);
  return taifmt(s, &now);
#if 0 /* ujr 2007-12-27 */
  taistore(buf, &now);

  s[0] = '@';
  for (i=0; i<8; i++) {
  	s[i*2+1] = hex[(buf[i] >> 4) & 15];
  	s[i*2+2] = hex[(buf[i] & 15)];
  }
  return 1+16; /* #bytes put in s */
#endif
}

#ifdef TAISTAMP
#include <time.h>
#include "print.h"
#include "simpleio.h"
int main(int argc, char **argv)
{ /* output taistamp, followed by args */
  char buf[64];

  putbuf(buf, taistamp(buf));

  if (*argv) argv++; /* skip arg0 */
  if (*argv) while (*argv) putfmt(" %s", *argv++);
  else { /* no args: translate to local time */
  	struct tai tai;
  	struct tm tm;
  	time_t t;
  	(void) taiscan(buf, &tai);
  	t = tailocal(&tai, &tm);
  	putfmt(" unix %d local ", (long) t);
  	putbuf(buf, printstm(buf, &tm));
  	putfmt(" %s", (tm.tm_isdst) ? tzname[1] : tzname[0]);
  }
  putbyte('\n');

  return 0;
}
#endif /* TAISTAMP */
