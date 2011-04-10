#include "pracc.h"
#include "skipline.h"
#include <errno.h>
#include <unistd.h>

/* return 0 if ok, 1 if no pracc file, -1 on error */

int readacct(const char *account, long *bp, long *lp)
{
  int oldfd;
  unsigned n;
  int c, partial;
  char buf[MAXLINE], *p;
  long lineno, value;
  long balance, limit;

  if (chdir(PRACCDIR) < 0) return -1;
  if ((oldfd = dup(0)) < 0) return -1;
  if (setin(account) < 0) {
  	(void) close(oldfd);
  	if (errno == ENOENT) return 1; /* no acct */
  	return -1; /* other file open error */
  }

  lineno = 0;
  balance = limit = 0;
  while (getline(p=buf, sizeof(buf), '\n', &partial) > 0) {
  	lineno += 1;
  	switch (c = *p++) {
  		case '-': /* debit line */
  			n = scanu(p, &value);
  			if (n > 0) balance -= value;
  			break;
  		case '+': /* credit line */
  			n = scanu(p, &value);
  			if (n > 0) balance += value;
  			break;
  		case '=': /* reset line */
  			n = scani(p, &value);
  			if (n > 0) balance = value;
  			break;
  		case '$': /* limit line */
  			n = scani(p, &value);
  			if (n > 0) limit = value;
  			else limit = UNLIMITED;
  			break;
  		default: /* skip */
  			break;
  	}
endline:
  	if (partial) skipline('\n');
  }

  if (dup2(oldfd, 0) < 0) return -1;

  if (bp) *bp = balance;
  if (lp) *lp = limit;

  return 0; /* ok */
}
