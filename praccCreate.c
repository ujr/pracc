#include "pracc.h"
#include "print.h"

#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

/*
 * Flags for open(2) when creating pracc files. We cannot use
 * fopen(3) because we need O_EXCL, which cannot be specified
 * using stdio...
 */
#define OPEN_TRUNC (O_WRONLY | O_CREAT | O_TRUNC)
#define OPEN_EXCL  (O_WRONLY | O_CREAT | O_EXCL)

/*
 * Create a new pracc file for the given account name
 * with the given initial balance and limit. The given
 * comment appears in the first comment line.
 *
 * If overwrite is 0, an existing pracc file is an error.
 * Otherwise, an existing pracc file will be overwritten.
 *
 * If successfully created, append an entry to the log.
 *
 * Return 0 if successful and -1 on error (see errno).
 *
 *   #pracc v2 ACCTNAME COMMENT
 *   $LIMIT @TAISTAMP USER minimum balance
 *   =BALANCE @TAISTAMP USER initial credit
 */
int praccCreate(const char *acctname, long balance, long limit,
                const char *username, const char *comment, int overwrite)
{
   time_t now;
   const char *fn;
   int fd, flags, saverr, len;
   char buf[3*MAXLINE], *p, *s;

   if (praccCheckName(acctname) < 0) return -1;

   now = time(0); // current time

   fn = praccPath(acctname);
   if (!fn) return -1; // ENOMEM

   flags = (overwrite) ? OPEN_TRUNC : OPEN_EXCL;
   fd = open(fn, flags, 0660);
   saverr = errno;
   free((void *) fn);
   errno = saverr;
   if (fd < 0) return -1; // FAILURE

   p = buf;
   p += prints(p, "#pracc v2 ");
   p += praccFormatName(p, acctname, MAXNAME);
   p += printc(p, ' ');
   p += praccFormatInfo(p, comment, MAXLINE-MAXNAME-32);
   p += printc(p, '\n');

   s = (limit == UNLIMITED) ? "unlimited" : "minimum balance";
   p += praccAssemble(p, '$', limit, now, username, s);
   p += praccAssemble(p, '=', balance, now, username, "initial credit");

   len = p - buf;
   if ((write(fd, buf, len) != len) || (fsync(fd) != 0)) {
      (void) close(fd);
      return -1; // see errno
   }
   close(fd); // already sync()ed

   return 0; // SUCCESS
}

