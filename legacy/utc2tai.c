/* utc2tai - copy stdin to stdout, replacing any occurrence
 *           of a valid utcscan() or oldscan() timestamp at
 *           the beginning of a line to a TAI timestamp.
 * ujr/2006-04-20 hacked to convert old log files.
 */
#include <time.h>
#include "scan.h"
#include "simpleio.h"
#include "tai.h"

int main(void)
{
  static char hex[16] = "0123456789abcdef";
  char *p, line[1024], buf[TAIBYTES];
  struct tm tm;
  struct tai tai;
  time_t t;
  int n, i, partial; 

  while ((n = getline(p=line, sizeof line, '\n', &partial)) > 0) {
  	if (((n=utcscan(p, &tm)) && (t=mktime(&tm)))
  	/* || ((n=oldscan(p, &tm)) && (t=mktime(&tm)))*/) {
  		tai.x = TAISHIFT + (uint64) t;
  		taistore(buf, &tai);
  		putbyte('@');
  		for (i=0; i<8; i++) {
  			putbyte(hex[(buf[i] >> 4) & 15]);
  			putbyte(hex[(buf[i] & 15)]);
  		}
  		p += n;
  	}
  	putline(p);
endline:
  	if (partial) logline("! line too long, truncated");
  	if (partial) skipline('\n');
  }
  if (n < 0) {
  	logline("! error reading stdin");
  	return 111;
  }
  return 0;
}
