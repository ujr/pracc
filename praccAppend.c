#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <pwd.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#include "pracc.h"
#include "tai.h"

/*
 * Append a pracc record by adding a line to the end
 * of the pracc file that belongs to the given acctname.
 *
 * Return 0 if successful and -1 on error (see errno).
 */
int praccAppend(const char *acctname, int type, long value,
                const char *username, const char *comment)
{
   struct passwd *pw;
   time_t now;
   const char *fn;
   char buf[MAXLINE];
   int fd, len, saverr;
   char *p;

   if (!username) {
      pw = getpwuid(getuid());
      if (pw == 0) return -1;
      username = pw->pw_name;
   }

   now = time(0);

/*
 * First, format all pieces of information into
 * a single line that is no longer than MAXLINE.
 */

   assert(1+10+1+TAISTAMPLEN+1+MAXNAME < sizeof(buf));
   len = praccAssemble(buf, type, value, now, username, comment);

/*
 * Now append the line just constructed
 * in only one write, that is, atomically!
 */

   fn = praccPath(acctname);
   if (!fn) return -1; // ENOMEM

   fd = open(fn, O_WRONLY | O_APPEND);
   saverr = errno;
   free((void *) fn); // release memory
   errno = saverr;
   if (fd < 0) return -1; // see errno

   if ((write(fd, buf, len) != len) || (fsync(fd) != 0)) {
      (void) close(fd);
      return -1; // see errno
   }
   close(fd); // already sync()ed

   return 0; // SUCCESS
}

