#include <sys/types.h>  // size_t, ssize_t
#include <unistd.h>     // write()

/**
 * On some special devices (notably terminals, networks, streams),
 * a write() operation may return less than specified. This isn't
 * an error and we should continue with the remainder of the data.
 * This phenomenon never happens with ordinary disk files.
 *
 * See Stevens (1993, p.406-408) for details.
 */
ssize_t
writen(int fd, const void *buf, size_t len)
{
   size_t nbytes = len;
   char * bufptr = (char *) buf;  // no ptr arith with void star

   while (nbytes > 0)
   {
      ssize_t n = write(fd, bufptr, nbytes);
      if (n <= 0) return -1; // see errno
      nbytes -= n;
      bufptr += n;
   }

   return len;
}
