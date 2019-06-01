/* netprint - send a print job to a networked printer
 *
 * Copyright (c) 2010-2013 by Urs-Jakob Ruetschi.
 * Use at your own exclusive risk and under the terms of the GNU
 * General Public License.  See AUTHORS, COPYRIGHT, and COPYING.
 */

// 2010-xx-xx      ?      coding
// 2010-12-21  2130-2330  coding
// 2010-12-22  2200-2300  coding
// 2011-01-01  1500-..    testing mit dem brother HL-2150N
// 2011-04-28  2330-2350  testing against netcat
// 2013-01-03  1100-1400  -m mode, invert loglevel, CUPSy log prefixes.

// TODO Add features from my original netprint: hexdump, transcript, pc only

#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "delay.h"
#include "ps.h"
#include "pjl.h"
#include "pracc.h" // only for praccIdentify()
#include "scan.h"
#include "writen.h"

#define min(x,y) ((x) < (y) ? (x) : (y))
#define max(x,y) ((x) > (y) ? (x) : (y))

#define DEFLT_WAIT0_PS 20    // pause before first pc probe
#define DEFLT_WAIT1_PS 10    // pause between more pc probes
#define DEFLT_WAIT0_PJL 300  // timeout for first PJL reply
#define DEFLT_WAIT1_PJL 120  // timeout for subsequent replies

#define SUCCESS 0            // exit code if successful
#define FAILSOFT 111         // exit code for temporary failure
#define FAILHARD 127         // exit code for permanent failure

enum {                       // How to do page counting:
   PCMODE_OFF = 0,           // -not at all
   PCMODE_PS = 1,            // -using PostScript commands
   PCMODE_PJL = 2,           // -using PJL JOB/EOJ and PAGE
   PCMODE_SNMP = 3,          // -using SNMP printer MIB
   PCMODE_UNKNOWN = -1       // -for internal use
};

enum {                       // States of a PJL job:
   PJL_STATE_INIT = 0,       // -initial
   PJL_STATE_SYNC,           // -after @PJL ECHO with our cookie
   PJL_STATE_JOB,            // -after "our" @PJL USTATUS JOB
   PJL_STATE_EOJ,            // -after "our" @PJL USTATUS EOJ
   PJL_STATE_PC2,            // -after 2nd @PJL INFO PAGECOUNT
   PJL_STATE_DONE            // -after 2nd pagecount arrived
};

enum {
   FATAL = 0,
   ERROR = 1,
   WARN  = 2,
   INFO  = 3,
   DEBUG = 4
};

void help(), usage(const char *fmt, ...), report(FILE *fp);
int tcpconnect(unsigned char ip[4], unsigned short port, int secs);
int tcplocal(int sockfd, unsigned char ip[4], unsigned short *port);
long sendjob(int jobfd, int devfd, int cookie);
void getstatus(int devfd, int cookie);
int handleinput(int devfd, int cookie);
void parseinput(const char *buf, unsigned len, int cookie);
void psinput(const char *buf, unsigned len, int cookie);
void pjlinput(const char *buf, unsigned len, int cookie);
int getmode(const char *s);
const char *getmodestr(int pcmode);
const char *getpjlstatestr(int pjlstate);
int writeall(int fd, const char *buf, unsigned len);
int fdblocking(int fd);
int fdnonblock(int fd);
char *progname(char **argv);
void logup(int level, const char *fmt, ...);
void die(int code, const char *fmt, ...);

char *me;                    // hello, my name is...
int loglevel = INFO;
int pcmode = PCMODE_OFF;     // how to do page counting
int wait0 = -1, wait1 = -1;  // delays waiting for page ejects
long pages = -1;             // #pages printed, initially unknown
long pc1 = -1, pc2 = -1;     // printer's pagecount, before and after
int pcpending;               // waiting for a PS pagecount report
int pjlstate;                // finite state machine for PJL job
long jobbytes = 0;           // #bytes of job data sent to printer
long devbytes = 0;           // #bytes read from printer

int
main(int argc, char *argv[])
{
   extern char *optarg;
   extern int optind, optopt, opterr;

   unsigned char ip[4];
   unsigned short port = 9100;
   int c, secs = 10;
   long number;

   int devfd;          // the network connection
   int cookie;         // for message authentication
   char *printer;      // printer address, eg, "1.2.3.4:9100"

   me = progname(argv);
   if (!me) return 127;

   opterr = 0; // prevent getopt output
   while ((c = getopt(argc, argv, "hm:qr:s:vV")) > 0) switch (c)
   {
      case 'h':
         help();
         return SUCCESS;
      case 'm':
         pcmode = getmode(optarg);
         if (pcmode == PCMODE_UNKNOWN)
           usage("unknown pagecount mode: -m '%s'", optarg);
         if (pcmode == PCMODE_SNMP)
           usage("-m SNMP is not yet implemented");
         break;
      case 'q':
         loglevel = FATAL;
         break;
      case 'v':
         loglevel += 1;
         break;
      case 'r':
         if (*optarg && optarg[scani(optarg, &number)] == '\0')
            wait0 = number;
         else usage("invalid argument to -r option: '%s'", optarg);
         break;
      case 's':
         if (*optarg && optarg[scani(optarg, &number)] == '\0')
            wait1 = number;
         else usage("invalid argument to -s option: '%s'", optarg);
         break;
      case 'V':
         praccIdentify("netprint");
         return SUCCESS;
      default:
         usage("invalid option: %c", optopt);
         return 127;
   }

   argc -= optind;
   argv += optind;

   if (*argv) printer = *argv++;
   else usage("printer not specified");

   if (scanip4op(printer, ip, &port) <= 0)
   {
      usage("Invalid ip and/or port for printer");
   }

   logup(INFO, "Connecting to %d.%d.%d.%d port %d ...",
         ip[0], ip[1], ip[2], ip[3], port);

   if ((devfd = tcpconnect(ip, port, secs)) < 0)
   {
      die(111, "Cannot connect");
   }

   if (tcplocal(devfd, ip, &port) < 0)
   {
      die(111, "Cannot get local address/port");
   }

   logup(DEBUG, "Local address is %d.%d.%d.%d port %d",
         ip[0], ip[1], ip[2], ip[3], port);

   srand(time(0) ^ getpid());
   cookie = 10000 + (rand() & 65535);

   /** Synchronise **/

   if (pcmode == PCMODE_PS)
   {
      psinit();
      if (pscount(devfd, cookie))
         die(127, "pscount");
      pcpending = 0;

      if (wait0 < 0) wait0 = DEFLT_WAIT0_PS;
      if (wait1 < 0) wait1 = DEFLT_WAIT1_PS;

      logup(DEBUG, "Page counting using PostScript, wait0=%d, wait1=%d",
                   wait0, wait1);
   }
   else if (pcmode == PCMODE_PJL)
   {
      pjlinit();
      if (pjluel(devfd))               // send a UEL
         die(127, "pjluel");
      if (pjlecho(devfd, cookie))      // @PJL ECHO cookie
         die(127, "pjlecho");
      if (pjlcount(devfd))             // @PJL INFO PAGECOUNT
         die(127, "pjlcount");
      if (pjljob(devfd, cookie, 0, 0)) // @PJL JOB cookie ...
         die(127, "pjljob");
      pjlstate = PJL_STATE_INIT;

      if (wait0 < 0) wait0 = DEFLT_WAIT0_PJL;
      if (wait1 < 0) wait1 = DEFLT_WAIT1_PJL;

      logup(DEBUG, "Page counting using PJL, wait0=%d, wait1=%d", wait0, wait1);
   }

   /** Send print job **/

   if (*argv) while (*argv)
   {
      char *jobfn = *argv++;
      int jobfd = open(jobfn, O_RDONLY);

      if (jobfd >= 0)
      {
         logup(INFO, "Sending %s to printer ...", jobfn);
         jobbytes += sendjob(jobfd, devfd, cookie);
      }
      else
      {
         logup(ERROR, "Open %s: %s", jobfn, strerror(errno));
      }
   }
   else
   {
      logup(INFO, "Sending stdin to printer ...");
      int jobfd = 0; // stdin is fd 0
      jobbytes += sendjob(jobfd, devfd, cookie);
   }

   logup(INFO, "Sent %d bytes job data to printer", jobbytes);

   /** Get job status **/

   if (pcmode != PCMODE_OFF)
   {
      logup(INFO, "Waiting for printer to finish ...");
      getstatus(devfd, cookie);
   }

   close(devfd);

   /** Report results **/

   logup(DEBUG, "Read %d bytes feedback from printer", devbytes);

   logup(INFO,
         "Done: pcmode=%s pages=%ld pc1=%ld pc2=%ld jobbytes=%ld inbytes=%ld",
         getmodestr(pcmode), pages, pc1, pc2);

   // This is the only line to stdout:
   report(stdout);
   
   return SUCCESS;
}

void
report(FILE *fp)
{
   fprintf(fp, "ok pcmode=%s", getmodestr(pcmode));
   fprintf(fp, " pages=");
   if (pages < 0) fprintf(fp, "?");
   else fprintf(fp, "%ld", pages);
   fprintf(fp, " pc1=");
   if (pc1 < 0) fprintf(fp, "?");
   else fprintf(fp, "%ld", pc1);
   fprintf(fp, " pc2=");
   if (pc2 < 0) fprintf(fp, "?");
   else fprintf(fp, "%ld", pc2);
   fprintf(fp, " jobbytes=%ld", jobbytes);
   fprintf(fp, " inbytes=%ld", devbytes);
   fprintf(fp, "\n");
   fflush(fp);
}

void
help()
{
   printf("Send files to printer and count pages printed.\n\n");
   printf("Usage: %s [options] host:port [files...]\n\n", me);
   printf("Options:\n");
   printf(" -h  print this help page\n");
   printf(" -m OFF   no page counting (default)\n");
   printf(" -m PS    use PostScript for page counting\n");
   printf(" -m PJL   use PJL for page counting\n");
   printf(" -m SNMP  use SNMP for page counting\n");
   printf(" -q  quiet (set log level to ERROR and above)\n");
   printf(" -r secs  how long to 'relax' after sending print job\n"); // wait0
   printf(" -s secs  how long to 'sleep' between probes/timeouts\n"); // wait1
   printf(" -v  increase verbosity (can be used several times)\n");
   printf(" -V  print version and exit\n\n");
   printf("The printer must be addressed using an IPv4 address,\n");
   printf("optionally followed by a colon and the port number\n");
   printf("(default is 9100). Example: 192.168.1.13:9101\n\n");
   printf("If no files are specified, send standard input.\n");
}

void
usage(const char *fmt, ...)
{
   char msg[256];
   va_list ap;

   if (fmt)
   {
      va_start(ap, fmt);
      vsnprintf(msg, sizeof(msg), fmt, ap);
      va_end(ap);

      fprintf(stderr, "%s: %s\n", me, msg);
   }

   fprintf(stderr, "Usage: %s [options] ip:port [files...]\n", me);
   fprintf(stderr, "Try option -h for help and -V for identification\n");

   exit(127); // FAILURE
}

int
tcpconnect(unsigned char ip[4], unsigned short port, int secs)
{
   struct sockaddr_in me;     // the local end
   struct sockaddr_in them;   // the remote end
   unsigned char localip[4];
   unsigned short localport;
   int rr, s;

   localip[0] = localip[1] = localip[2] = localip[3] = 0;
   localport = 0;

   // Get a socket descriptor:
   s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
   if (s < 0) return -1; // can't get socket, see errno
   if (fdnonblock(s) < 0) goto errout;

   // Bind to local address, if any:
   memset(&me, 0, sizeof(me));
   me.sin_family = AF_INET;
   me.sin_port = htons(localport);
   memcpy(&me.sin_addr, localip, 4);
   rr = bind(s, (struct sockaddr *) &me, sizeof(me));
   if (rr < 0) goto errout;

   // Connect to remote address:
   memset(&them, 0, sizeof(them));
   them.sin_family = AF_INET;
   them.sin_port = htons(port);
   memcpy(&them.sin_addr, ip, 4);

   // Some connect() errors:
   // -EINPROGRESS: operation now in progress
   // -EALREADY: operation already in progress
   // -EISCONN: endpoint already connected (good)
   // -ETIMEDOUT: connection timed out
   // -ECONNREFUSED: connection refused
   // -ENETUNREACH: network unreachable

   while (1)
   {
      errno = 0;
      rr = connect(s, (struct sockaddr *) &them, sizeof(them));
      if ((rr == 0) && (errno == 0)) break; // really no error: good
      if (errno == EISCONN) break; // endpoint already connected: good
      if ((errno == EALREADY) || (errno == EINPROGRESS)) { } // nix
      else goto errout;
      if (secs > 0) { secs -= 1; delay(1); }
      else { errno = ETIMEDOUT; goto errout; }
   }

   if (fdblocking(s) < 0) goto errout;

   return s; // connected and non-blocking

errout:
   rr = errno;
   (void) close(s);
   errno = rr;
   return -1; // see errno
}

int
tcplocal(int sockfd, unsigned char ip[4], unsigned short *port)
{
   struct sockaddr_in sa;
   socklen_t dummy = sizeof(sa);

   if (getsockname(sockfd, (struct sockaddr *) &sa, &dummy) < 0)
      return -1;
   memcpy(ip, (const void *) &sa.sin_addr, 4);
   if (port) *port = ntohs(sa.sin_port);

   return 0; // ok
}

/*
 * Send print job data to printer and handle data sent back
 * by printer. This is a classical select() loop, reading from
 * two file descriptors (jobfd, devfd) and writing to one (devfd).
 */
long
sendjob(int jobfd, int devfd, int cookie)
{
   int nfds;
   fd_set rfds, wfds;
   char buffer[8192];
   char *bufptr = buffer;
   long bytes = 0, total = 0;

   if (fdnonblock(devfd) < 0) // want non-blocking printer i/o
      die(127, "cannot set O_NONBLOCK on devfd");
   if (fdnonblock(jobfd) < 0) // want non-blocking job reading
      die(127, "cannot set O_NONBLOCK on jobfd");

   // for select(): one more than the largest fd:
   nfds = 1 + ((jobfd > devfd) ? jobfd : devfd);

   while (1)
   {
      FD_ZERO(&rfds);
      FD_ZERO(&wfds);

      if (bytes > 0) {
         FD_SET(devfd, &wfds);
         FD_SET(devfd, &rfds);
      }
      else {
         FD_SET(jobfd, &rfds);
      }

      if (select(nfds, &rfds, &wfds, NULL, NULL) < 0)
      {
         die(127, "sendjob's call to select");
         // Note: cupspracc sleeps 2 secs and tries again...?
      }

      // Async printer input:

      if (FD_ISSET(devfd, &rfds))
      {
         handleinput(devfd, cookie);
      }

      // Read print job data:

      if ((bytes == 0) && FD_ISSET(jobfd, &rfds))
      {
         ssize_t n = read(jobfd, buffer, sizeof(buffer));
         if (n > 0) {bytes = n;logup(DEBUG,"Read %d bytes job data",n);}
         else if (n < 0) {
            if (errno == EAGAIN || errno == EINTR) bytes = 0;
            else { // all other errors:
               logup(ERROR, "sendjob: read(jobfd): %s", strerror(errno));
               return -1; // see errno
            }
         }
         else break; // end-of-file, we're done

         bufptr = buffer; // reset bufptr
      }

      // Write job data to printer:

      if ((bytes > 0) && FD_ISSET(devfd, &wfds))
      {
         ssize_t n = write(devfd, bufptr, bytes);
         if (n < 0) logup(ERROR, "sendjob: write(devfd): %s", strerror(errno));
         else {
            bytes -= n;
            bufptr += n;
            total += n;
         }
      }
   }

   return total; // #bytes sent to printer
}

void
getstatus(int devfd, int cookie)
{
   long lastpages, i;
   struct timeval timeout;

   if (pcmode == PCMODE_PS)
   {
      for (i = wait0; i > 0; i = sleep(i));
      if (pscount(devfd, cookie) < 0)
         die(127, "pscount");
      lastpages = pages - 1; // -2
      timeout.tv_sec = wait1;
   }
   else if (pcmode == PCMODE_PJL)
   {
      // Note: cupspracc sends jobid instead of cookie
      if (pjleoj(devfd, cookie) < 0)
         die(127, "pjleoj");
      timeout.tv_sec = wait0;
   }
   //else if (pcmode == PCMODE_SNMP) TODO

   while (1)
   {
      fd_set rfds;
      int r;

      // Revert to non-blocking printer i/o:
      if (fdnonblock(devfd) < 0)
         die(127, "cannot set O_NONBLOCK on devfd");

      FD_ZERO(&rfds);
      FD_SET(devfd, &rfds);
      timeout.tv_usec = 0;

      // Wait until something to read or timeout:
      if ((r = select(devfd+1, &rfds, NULL, NULL, &timeout)) <= 0)
      {
         if (r == 0) logup(DEBUG, "Timeout waiting for printer");
         else logup(ERROR, "select failed");
         break; // error or timeout: silently give up
      }

      // Read the printer; break on EOF or error:
      if (handleinput(devfd, cookie) <= 0) break;

      if (pcmode == PCMODE_PS)
      {
         if (pcpending && (pages == lastpages)) break;
         if (pages > 0) logup(DEBUG, "Pages printed: %d", pages);
         for (i = wait1; i > 0; i = sleep(i));
         if (pscount(devfd, cookie) < 0) die(127, "pscount");
         lastpages = pages;
         pcpending = 0;
      }
      else if (pcmode == PCMODE_PJL)
      {
         if (pjlstate == PJL_STATE_EOJ)
         {
            // After @PJL USTATUS EOJ, send another @PJL INFO PAGECOUNT:
            if (pjlcount(devfd) < 0) die(127, "pjlcount");
            pjlstate = PJL_STATE_PC2;
         }
         else if (pjlstate == PJL_STATE_PC2)
         {
            if (pc1 >= 0 && pc2 > pc1 + pages) {
               logup(DEBUG, "Setting pages from pc delta (pages "
                            "was %d, delta is %d)", pages, pc2 - pc1);
               pages = pc2 - pc1;
            }
            pjlstate = PJL_STATE_DONE;
            break;
         }
      }
      //else if (pcmode == PCMODE_SNMP) TODO

      // Reset the select timeout:
      timeout.tv_sec = wait1;
      timeout.tv_usec = 0;
   }

   if (pcmode == PCMODE_PJL)
   {
      // Errors, typically broken pipe, are non-fatal here
      // because the printer may closed the connection by now:
      if (pjloff(devfd)) logup(ERROR, "pjloff");
      if (pjluel(devfd)) logup(ERROR, "pjluel");
   }
}

/*
 * To be called after select() indicated that there's
 * something to read from the given devfd. Return nbytes.
 */
int
handleinput(int devfd, int cookie)
{
   char buf[1024];
   ssize_t nbytes;

   nbytes = read(devfd, buf, sizeof(buf));

   if (nbytes > 0)
   {
      parseinput(buf, nbytes, cookie);
      devbytes += nbytes;
   }
   else if (nbytes == 0)
   {
      // After successful select(), this means EOF:
      logup(DEBUG, "Printer closed connection");
   }
   else
   {
      logup(ERROR, "Reading printer failed: %s", strerror(errno));
   }

   return nbytes;
}

/*
 * Handle input from the printer: write to log file and
 * look for messages (PostScript or PJL) and parse them.
 */
void
parseinput(const char *buf, unsigned len, int cookie)
{
   char out[64];
   int i, n;

   n = min(sizeof(out) - 4, len);

   for (i = 0; i < n; i++)
   {
      register char c = buf[i];
      out[i] = ((c ==  ' ') || isgraph(c)) ? c : '.';
   }

   if (len > n)
   {
      out[i++] = '.'; out[i++] = '.'; out[i++] = '.';
   }

   out[i] = '\0'; // terminate string

   logup(DEBUG, "Got %d bytes from printer: %s", len, out);

   switch (pcmode)
   {
      case PCMODE_PS:
         psinput(buf, len, cookie);
         break;
      case PCMODE_PJL:
         pjlinput(buf, len, cookie);
         break;
      default: /* ignore */ ;
   }
}


void
psinput(const char *buf, unsigned len, int cookie)
{
   register const char *p = buf;
   const char *end = buf + len;
   // global: pc1, pc2, pages

   while (p < end) {
      int t = pschar(*p++);
      if (t) switch (t) {
      case PS_MSG_PAGECOUNT:
         if (ps_cookie == cookie) {
            logup(DEBUG, "psinput: PAGECOUNT %d", ps_pagecount);
            if (pc1 < 0) pc1 = ps_pagecount; // initial pc
            else if (ps_pagecount >= pc1) {
               pc2 = ps_pagecount;
               pages = pc2 - pc1;
            }
            pcpending = 1; // got valid pc message
         }
         else logup(DEBUG, "psinput: got cookie %ld, expected %ld",
                    ps_cookie, cookie);
         break;

      case PS_MSG_PRERROR:
         logup(ERROR, "PrinterError: %s", ps_error);
         exit(1); //CUPS_BACKEND_FAILED
         break;

      case PS_MSG_FLUSHING:
         logup(ERROR, "Flushing: rest of job will be ignored");
         exit(2); //CUPS_BACKEND_CANCEL // cancel job
         break;
      }
   }
}

/*
 * Parse PJL messages and try counting pages printed.
 * Use the messages to advance in a simple state diagram:
 *
 *   INIT---(1)-->SYNCED---(2)-->JOB---(3)-->EOJ---(4)-->DONE
 *
 * Transitions: (1) ECHO with our cookie; (2) JOB with
 * our cookie; (3) EOJ with our cookie; (4) 2nd PAGECOUNT.
 *
 * While INJOB, use all PAGE and EOJ messages to set
 * the global pages variable, not just our EOJ message.
 * Reason: at least the HP LaserJet 5000 gets the EOJ
 * pages wrong for nested JOB/EOJ pairs; to reproduce,
 * print a Windows Test Page to an HP LaserJet 5000...
 */
// globals: pjlstate, pc1, pc2, pages
void
pjlinput(const char *buf, unsigned len, int cookie)
{
   register const char *p = buf;
   const char *end = buf + len;

   while (p < end) switch (pjlchar(*p++))
   {
      case PJL_MSG_COOKIE:
         if (pjl_cookie != cookie) break; // not our cookie, silently ignore
         if (pjlstate != PJL_STATE_INIT) break;
         pjlstate = PJL_STATE_SYNC;
         logup(DEBUG, "pjlinput: got ECHO %ld (our cookie), state %s -> %s",
               pjl_cookie, getpjlstatestr(PJL_STATE_INIT),
                           getpjlstatestr(pjlstate));
         break;

      case PJL_MSG_PAGECOUNT:
         if (pjlstate >= PJL_STATE_SYNC && pjlstate < PJL_STATE_DONE)
         {
            if (pc1 < 0) pc1 = pjl_pagecount; // only once
            else if (pjl_pagecount > pc2) pc2 = pjl_pagecount;
            logup(DEBUG, "pjlinput: got PAGECOUNT %ld in state %s, "
                         "pc1=%ld, pc2=%ld",
                  pjl_pagecount, getpjlstatestr(pjlstate), pc1, pc2);
         }
         else
         {
            logup(DEBUG, "pjlinput: got PAGECOUNT %ld in state %s, ignore",
                  pjl_pagecount, getpjlstatestr(pjlstate));
         }
         break;

      case PJL_MSG_JOBSTART:
         if ((pjlstate == PJL_STATE_SYNC) && (pjl_jobnum == cookie))
         {
            pjlstate = PJL_STATE_JOB;

            logup(DEBUG, "pjlinput: got JOB %ld (our cookie), state %s -> %s",
                  pjl_jobnum, getpjlstatestr(PJL_STATE_SYNC),
                              getpjlstatestr(pjlstate));
         }
         else
         {
            int ourcookie = pjl_jobnum == cookie;
            logup(DEBUG, "pjlinput: got JOB %ld (%s) in state %s, ignore",
                  pjl_jobnum, ourcookie ? "our cookie" : "bad cookie",
                  getpjlstatestr(pjlstate));
         }
         break;

      case PJL_MSG_JOBEND:
         if ((pjlstate == PJL_STATE_JOB) && (pjl_jobnum == cookie))
         {
            pjlstate = PJL_STATE_EOJ;
            pages = max(pjl_numpages, pages);

            logup(DEBUG, "pjlinput: got EOJ %ld (our cookie) %ld, "
                         "state %s -> %s, pages=%ld",
                  pjl_jobnum, pjl_numpages,
                  getpjlstatestr(PJL_STATE_JOB),
                  getpjlstatestr(pjlstate), pages);
         }
         else
         {
            int ourcookie = pjl_jobnum == cookie;
            logup(DEBUG, "pjlinput: got EOJ %ld (%s) %ld in state %s, ignore",
                  pjl_jobnum, ourcookie ? "our cookie" : "bad cookie",
                  pjl_numpages, getpjlstatestr(pjlstate));
         }
         break;

      case PJL_MSG_PAGE:
         if (pjlstate == PJL_STATE_JOB)
         {
            pages = max(pjl_curpage, pages);
            logup(DEBUG, "pjlinput: got PAGE %ld in state %s, pages=%ld",
                  pjl_curpage, getpjlstatestr(pjlstate), pages);
            logup(INFO, "Printed page %d", pjl_curpage);
         }
         else
         {
            logup(DEBUG, "pjlinput: got PAGE %ld in state %s, ignore",
                  pjl_curpage, getpjlstatestr(pjlstate));
         }
         break;
   }
}

/*
 * Write the given buffer to the given file descriptor in blocking
 * mode: do not return until all data has been written or an error
 * occurred.
 *
 * Use this function for sending small amounts of data to the printer
 * en bloc without bothering about the printer's asynchronous replies.
 * Careful: this could create a dead-lock (no select-loop)!
 *
 * Return 0 if OK or -1/errno if an error occurred.
 */
int
writeall(int fd, const char *buf, unsigned len)
{
   if (fdblocking(fd) < 0) return -1; // see errno

   if (writen(fd, buf, len) < 0) return -1; // see errno

   return 0; // ok
}

int
getmode(const char *s)
{
   assert(s != NULL);
   if (strcasecmp(s, "OFF") == 0)  return PCMODE_OFF;
   if (strcasecmp(s, "NONE") == 0) return PCMODE_OFF;
   if (strcasecmp(s, "PS") == 0)   return PCMODE_PS;
   if (strcasecmp(s, "PJL") == 0)  return PCMODE_PJL;
   if (strcasecmp(s, "SNMP") == 0) return PCMODE_SNMP;
   return PCMODE_UNKNOWN;
}

const char *
getmodestr(int pcmode)
{
   switch (pcmode)
   {
      case PCMODE_OFF:   return "OFF";
      case PCMODE_PS:    return "PS";
      case PCMODE_PJL:   return "PJL";
      case PCMODE_SNMP:  return "SNMP";
   }

   return "UNKNOWN";
}

const char *
getpjlstatestr(int pjlstate)
{
   switch (pjlstate)
   {
      case PJL_STATE_INIT:  return "INIT";
      case PJL_STATE_SYNC:  return "SYNC";
      case PJL_STATE_JOB:   return "JOB";
      case PJL_STATE_EOJ:   return "EOJ";
      case PJL_STATE_PC2:   return "PC2";
      case PJL_STATE_DONE:  return "DONE";
   }

   return "?";
}

/*
 * These two functions take a descriptor of an open file (or pipe,
 * socket, etc) and set/clear the O_NONBLOCK bit in the status flags.
 * Return -1 on error, some other value if ok.
 */

#ifndef O_NONBLOCK
#error Your system headers do not define O_NONBLOCK.
#endif

int
fdblocking(int fd)
{
   return fcntl(fd, F_SETFL, fcntl(fd, F_GETFL, 0) & ~O_NONBLOCK);
}

int
fdnonblock(int fd)
{
   return fcntl(fd, F_SETFL, fcntl(fd, F_GETFL, 0) | O_NONBLOCK);
}

/*
 * Get the program name: the basename of the first
 * entry in argv, the array of command line arguments.
 *
 * Return pointer to progname or NULL if no such argument.
 */
char *
progname(char **argv)
{
   const char *s = 0;
   register const char *p;

   if (argv && *argv)
   {
      // make s point to one after the last slash in argv[0]:
      p = s = *argv;
      while (*p) if (*p++ == '/') if (p) s = p; // advance
   }

   return (char *) s;
}

void
logup(int level, const char *fmt, ...)
{
   va_list ap;
   const char *prefix;
   static char *prefixes[] = {
      "FATAL: ",
      "ERROR: ",
      "WARN:  ",
      "INFO:  ",
      "DEBUG: "
   };

   va_start(ap, fmt);

   if (level > loglevel) return;

   // Ugly, but better safe than sorry:
   // Be sure we don't get an array index out of bounds below!

   assert(FATAL == 0);
   assert(DEBUG == 4);

   if (level < FATAL) level = FATAL;
   if (level > DEBUG) level = DEBUG;

   prefix = prefixes[level];
   //prefix = "FEWID"[level];

   fprintf(stderr, "%s", prefix);
   vfprintf(stderr, fmt, ap);
   fprintf(stderr, "\n");

   va_end(ap);
}

void
die(int code, const char *fmt, ...)
{
   char msg[256];
   int saverr = errno;

   va_list ap;
   va_start(ap, fmt);

   vsnprintf(msg, sizeof(msg), fmt, ap);

   if (saverr == 0) logup(FATAL, "%s", msg);
   else logup(FATAL, "%s: %s", msg, strerror(errno));
   
   va_end(ap);

   exit(code);
}
