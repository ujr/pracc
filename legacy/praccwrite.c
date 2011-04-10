#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>

#include "pracc.h"
#include "print.h"
#include "taistamp.h"

int praccwrite(const char *account, int type,
   long number, const char *user, const char *info)
{
   char buf[MAXLINE];
   char *p, *bufend;
   const char *fn;
   int fd, len, saverr;

   assert(sizeof(buf) > 1+10+1+TAISTAMPLEN+1);

   switch (type) {
      case '-': /* debit */ break;
      case '+': /* credit */ break;
      case '=': /* reset */ break;
      case '$': /* limit */ break;
      case '!': /* error */ break;
      case '#': /* comment */ break;
      default: abort(); /* BUG */
   }

   fn = praccpath(account);
   if (fn == 0) return -1; // ENOMEM
   fd = open(fn, O_WRONLY | O_APPEND);
   saverr = errno;
   free((void *) fn);
   errno = saverr;
   if (fd < 0) return -1; // see errno

   p = buf;
   bufend = buf + sizeof(buf) - 1; // reserve one byte for \n
   p += printc(p, (char) type); // debit, error, etc
   switch (type) {
   case '-': // debit
   case '+': // credit
      p += printu(p, number);
      break;
   case '=': // reset
      p += printi(p, number);
      break;
   case '$': // limit
      if (number <= UNLIMITED)
         p += printc(p, '*');
      else p += printi(p, number);
      break;
   case '#': // notes do not have time stamps!
      goto justinfo;
   }
   p += printc(p, ' ');
   p += taistamp(p);
   p += printc(p, ' ');
   p += praccuser(p, user, MAXNAME);
justinfo:
   p += printc(p, ' ');
   if (info) while ((p < bufend) && *info) *p++ = *info++;
   p += printc(p, '\n');
  
   /*
    * Now append the line just constructed
    * in just one write, that is, atomically.
    */

   len = p - buf;
   if (write(fd, buf, len) != len) return -1;

   if (fsync(fd) < 0) return -1; // flush cache to disk
   if (close(fd) < 0) return -1; // require proper close

   return 0; // SUCCESS
}
