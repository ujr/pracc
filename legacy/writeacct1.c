#include <assert.h>
#include <time.h> /* V1 */
#include <unistd.h>

#include "open.h"
#include "pracc.h"
#include "print.h"
#include "utcstamp.h"

/*
 * This is like writeacct(), but produces pracc v1 records
 */

int writeacct1(const char *account, int type, long number, const char *info)
{
  int praccfd;
  char buf[MAXLINE];
  char *bufptr, *bufend;
  time_t tl; /* V1 */
  struct tm *tp; /* V1 */

  assert(sizeof(buf) > 1+10+26); /* worst case mandatory part */ /* V1 */

  switch (type) {
  	case '-': /* debit */ break;
  	case '+': /* credit */ break;
  	case '=': /* reset */ break;
  	//case '$': /* limit */ break; /* V1 has no limit */
  	case '?': /* pracc error */ break;
  	case '!': /* print error */ break;
  	case '#': /* comment */ break;
  	default: abort(); /* BUG */
  }

  if (chdir(PRACCDIR) != 0) return -1;
  praccfd = open_pracc(account);
  if (praccfd < 0) return -1;

  tl = time(0);
  tp = localtime(&tl);

  bufptr = buf;
  bufend = buf + sizeof(buf) - 1; /* reserve one byte for \n */
  bufptr += printc(bufptr, (char) type); /* debit, error, etc */
  if ((type == '$') && (number < UNLIMITED)) bufptr += printc(bufptr, '*');
  else if (type != '#') bufptr += printu(bufptr, number); /* no num for # */
  bufptr += prints(bufptr, " date ");
  bufptr += printu(bufptr, tp->tm_year+1900);
  bufptr += printc(bufptr, '-');
  tp->tm_mon += 1;
  if (tp->tm_mon < 10) bufptr += printc(bufptr, '0');
  bufptr += printu(bufptr, tp->tm_mon);
  bufptr += printc(bufptr, '-');
  if (tp->tm_mday < 10) bufptr += printc(bufptr, '0');
  bufptr += printu(bufptr, tp->tm_mday);
  bufptr += printc(bufptr, ' ');
  if (tp->tm_hour < 10) bufptr += printc(bufptr, '0');
  bufptr += printu(bufptr, tp->tm_hour);
  bufptr += printc(bufptr, ':');
  if (tp->tm_min < 10) bufptr += printc(bufptr, '0');
  bufptr += printu(bufptr, tp->tm_min);
  bufptr += printc(bufptr, ':');
  if (tp->tm_sec < 10) bufptr += printc(bufptr, '0');
  bufptr += printu(bufptr, tp->tm_sec);
  bufptr += printc(bufptr, ' ');
  if (info) while ((bufptr < bufend) && *info) *bufptr++ = *info++;
  bufptr += printc(bufptr, '\n');

  /* append atomically: in just one write! */
  if (write(praccfd, buf, bufptr-buf) != bufptr-buf) return -1;
  if (close(praccfd) < 0) return -1;

  return SUCCESS;
}
