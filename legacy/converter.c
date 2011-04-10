/* converter.c */
/* Copyright (c) 2006 by Urs-Jakob Rueetschi */

#include <errno.h>
#include <string.h>
#include <time.h>

#include "pracc.h"
#include "scan.h"
#include "scf.h"
#include "simpleio.h"
#include "skipline.h"
#include "streq.h"
#include "timestamp.h"

int unix2tai(time_t t, char s[1+TAIBYTES+TAIBYTES]);
void syntax(const char *s);

char *me;
char line[MAXLINE];
long lineno = 0;
char buf[64];

int main(int argc, char **argv)
{
  int n, partial;
  struct tm tm, *tp;
  struct tai tai;
  time_t t;
  char *p;

  me = scfbase(argv);
  if (!me) return 127; /* no arg0 */

  /*if (utcinit() < 0)
  	die1(111, "cannot setenv TZ=\"\"");*/
  if (setin(PRACCLOG) < 0) /* stdin from pracc file */
  	die2(111, "canont open ", PRACCLOG);

  while ((n=getline(p=line, sizeof(line), '\n', &partial)) > 0) {
  	lineno += 1;
  	/*DEBUG logfmt("line %d: %s", lineno, p);*/

  	while (isspace(*p)) ++p;
  	if (((n = utcscan(p, &tm)) && (t = mktime(&tm))) ||
  	    ((n = taiscan(p, &tai)) && ((t = tailocal(&tai, &tm))>0)) ||
  	    ((n = oldscan(p, &tm)) && (t = mktime(&tm)))) {
  		p += n;

		/* convert time_t to TAI and write to stdout */
		putbuf(buf, unix2tai(t, buf));
  		putline(p); // rest of line
  	}
  	else syntax("invalid timestamp");

endline: /* skip remainder of long lines */
  	if (partial) syntax("line too long");
  	if (partial) skipline('\n');
  }
  if (n < 0)
  	die2(111, "error reading from ", PRACCLOG);
  return 0;
}

int unix2tai(time_t t, char s[1+TAIBYTES+TAIBYTES])
{
  static char hex[16] = "0123456789abcdef";
  struct tai tt;
  char buf[TAIBYTES];
  int i;
  
  tt.x = t + TAISHIFT;
  taistore(buf, &tt);

  s[0] = '@';
  for (i=0; i<8; i++) {
	  s[i*2+1] = hex[(buf[i] >> 4) & 15];
	  s[i*2+2] = hex[(buf[i] & 15)];
  }
  return 1+16;
}

void syntax(const char *s)
{
  if (!s) s =  "syntax error";
  logfmt("%s: %s line %d: %s\n", me, PRACCLOG, lineno, s);
}

void die4(int code, char *s1, char *s2, char *s3, char *s4)
{
  logfmt("%s: ", me);
  if (s1) logstr(s1);
  if (s2) logstr(s2);
  if (s3) logstr(s3);
  if (s4) logstr(s4);
  if (errno) {
  	logstr(": ");
  	logstr(strerror(errno));
  }
  logchar('\n');
  exit(code);
}
