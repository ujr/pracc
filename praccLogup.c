#include "pracc.h"

#include <assert.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>

#include "print.h"
#include "tai.h"

/*
 * Append a line to the common pracc log. Line format:
 *
 *   @timestamp by USER acct NAME: INFO
 *
 * where INFO depends on the tool that appends the line
 * and should briefly describe what was done to acct NAME.
 */
int praccLogup(const char *username, const char *acctname, const char *info)
{
   struct tai now;
   char buf[MAXLINE];
   register char *p;
   const char *endp;
   int fd, len;

   assert(username && acctname && info);
   assert(TAISTAMPLEN+4+MAXNAME+6+MAXNAME+2+1 < sizeof(buf));

   tainow(&now); // system time

   p = buf;
   endp = buf + sizeof(buf) - 1;

   p += taifmt(p, &now);
   p += prints(p, " by ");
   p += praccFormatName(username, p, MAXNAME);
   p += prints(p, " acct ");
   p += praccFormatName(acctname, p, MAXNAME);
   p += prints(p, ": ");
   p += praccFormatInfo(info, p, endp-p);
   p += printc(p, '\n');

   len = p - buf;
   fd = open(PRACCLOG, O_WRONLY | O_APPEND | O_CREAT, 0660);
   if (fd < 0) return -1; // see errno

   if (write(fd, buf, len) != len) return -1;
   if (close(fd) < 0) return -1;

   return 0; // SUCCESS
}

