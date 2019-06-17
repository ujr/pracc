/*
 * Wait for <nsec> seconds using the select system call as a timer
 * independent of the alarm/SIGALRM method. This is an "officially
 * sanctioned" method according to the select(2) man page on Linux.
 *
 * I'm unsure, however, about non-Linux systems...      XXX
 *
 * Another possibility to implement this is to set a timer/alarm
 * and then to pause(2), which yields the processor and returns
 * only on a signal, hopefully our SIGALRM. Note that sleep(3)
 * usually is implemented along these lines.  This is fragile
 * because SIGALRM is no longer available for other purposes.
 *
 * A negative retval with errno == EINTR signals that delay()
 * was interrupted and most likely waited for less than <nsec>
 * seconds! Normally, delay() returns zero.
 */

#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

int delay(unsigned nsec)
{
  struct timeval tv;

  tv.tv_sec = nsec;    /* seconds */
  tv.tv_usec = 0;      /* micro seconds */

  return select(0, NULL, NULL, NULL, &tv);
}
