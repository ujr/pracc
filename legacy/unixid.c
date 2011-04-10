/* unixid.c - part of netprint source
 *
 * Copyright (c) 2003 by Urs-Jakob Ruetschi.
 * Free software under the terms of the GNU General Public License.
 * See the files AUTHORS, COPYRIGHT, and COPYING.
 */

#include <sys/types.h>     /* gid_t, uid_t */
#include <unistd.h>        /* getgroups */

#include "print.h"

int unixid(char *buf, unsigned max);

/* Render a text representation of the process' UID,
 * GID, and auxiliary GIDs into the buffer provided.
 * Append 0 if enough space. Return length of string.
 *
 * Example: "uid 510 gid 456 aux 14 15 129 132" [\0]
 */
int unixid(char *buf, unsigned max)
{
  gid_t gidlist[256]; /* hopefully enough */
  uid_t uid = getuid();
  gid_t gid = getgid();

  //int i, num = getgroups(0, gidlist);
  int i, num = getgroups(256, gidlist);
  register char *p = buf, *q = p + max;

  if (max < 4 + printu(0, (unsigned long) uid))
  	return 0;  /* not enough space for uid */
  p += prints(p, "uid ");
  p += printu(p, (unsigned long) uid);

  if (max < (p-buf) + 5 + printu(0, (unsigned long) gid))
  	return p - buf;  /* not enough space for gid */
  p += prints(p, " gid ");
  p += printu(p, (unsigned long) gid);

  if (max < (p-buf) + 4 + num*(1+10))
  	return p - buf;  /* not enough space for all aux gids */
  if (num > 0) p += prints(p, " aux");
  for (i = 0; i < num; i++) {
  	if (gidlist[i] == gid) continue; /* skip gid */
  	p += printc(p, ' ');
  	p += printu(p, (unsigned long) gidlist[i]);
  }

  if (max > (p-buf)) *p = '\0';
  return p - buf;
}

#ifdef TEST
/* compile: cc -DTEST -o unixid unixid.c myclib/print?.c */
int main(int argc, char **argv)
{
  char buf[256];
  int len = sizeof buf;
  if (argv[1]) { len = atoi(argv[1]);
  	if (len > sizeof buf) len = sizeof buf;
  	write(1, buf, prints(buf, "buf size: "));
  	write(1, buf, printu(buf, len));
  	write(1, "\n", 1);
  }
  len = unixid(buf, len);
  write(1, buf, len); write(1, "\n", 1);
  return 0;
}
#endif /* TEST */
