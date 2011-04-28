/* Pracc as a CUPS backend
 *
 * Copyright (c) 2007-2011 by Urs-Jakob Ruetschi.
 * Use at your own exclusive risk and under the terms of the GNU
 * General Public License.  See AUTHORS, COPYRIGHT, and COPYING.
 */

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>
#include <cups/cups.h>
#include <cups/http.h>
#include <cups/backend.h>

#include "delay.h"
#include "pracc.h"
#include "print.h"
#include "tai.h"

#include "ps.h"
#include "pjl.h"

/* Configuration */

#define PRIVATE_LOGFILE "/var/log/cups/pracc" // XXX

#define DEFLT_WAIT0_PS 20    /* pause before first pc probe */
#define DEFLT_WAIT1_PS 10    /* pause between more pc probes */
#define DEFLT_WAIT0_PJL 300  /* timeout for first PJL reply */
#define DEFLT_WAIT1_PJL 120  /* timeout for subsequent replies */

#define SHELL "/bin/sh"      /* Used to launch the job scanner */

/* Type definitions */

typedef enum {                 // How to do accounting:
   NONE = 0,                   // no accounting
   POSTSCRIPT = 1,             // use PostScript commands
   PJL = 2,                    // use PJL JOB/EOJ and PAGE
   JOBSCAN = 3                 // just scan print job
} mode;

enum {                         // States of a PJL job:
   PJLINIT = 0,                // initial state
   PJLSYNC,                    // after @PJL ECHO with our cookie
   PJLJOB,                     // after "our" @PJL USTATUS JOB
   PJLDONE                     // after "our" @PJL USTATUS EOJ
};

/* Global variables */

int pagecost = 0;              // cost per printed page
mode acctmode = NONE;          // no accounting by default
int wait0 = -1, wait1 = -1;    // delays waiting for page ejects
const char *jobscan = 0;       // optional job page scanner

const char *acctname;          // name of account to charge
char jobuser[MAXNAME];         // job user taken from argv[2]
char jobtitle[48];             // job title taken from argv[3]
const char *printer;           // printer name (not hostname)
long pages = -1;               // num pages printed by printer
long pagecount = -1;           // printer's pagecount register
long jobid = -1;               // job id number: atoi(argv[1])
long jobpages = -1;            // num pages in print job

int cookie;                    // for message authentication
int pcpending;                 // 1 if pending PS pc message
int pjlstate;                  // state for PJL job; see pjlinput()

int jobfd;                     // print job's file descriptor
int devfd;                     // printer's file descriptor

char hostname[1024];           // where to connect to,
char portname[256];            //  to what service (port name/num),
char username[256];            //  and as what user

/* Prototypes */

void parseURI(const char *deviceURI);
mode parseMode(const char *value, mode deflt);
int parseOnOff(const char *value, int deflt);

void parseinput(const char *buf, unsigned len);
void pjlinput(const char *buf, unsigned len);
void psinput(const char *buf, unsigned len);

int copyuser(char *dest, int size, const char *s);
int copytitle(char *dest, int size, const char *s);
void checkaccess(const char *username, const char *acctname);
long sendjob(int jobfd, int devfd);
long runscan(int fd, const char *scanprog);
long estimate(long m, long n);
void acctstr(char *buf, int maxlen);
int logpc(long pc, const char *printer);

int writeall(int fd, const char *buf, unsigned len);
ssize_t writen(int fd, const void *buf, size_t len);
int fdblocking(int fd);
int fdnonblock(int fd);

char *cupsGetJobBilling(const char *printer, long jobid);

void log_page(long number, long copies);   // PAGE: ...
void log_state(const char *s);             // STATE: ...
void log_debug(const char *fmt, ...);      // DEBUG: ...
void log_info(const char *fmt, ...);       // INFO: ...
void log_error(const char *fmt, ...);      // ERROR: ...
void die(int code, const char *fmt, ...);  // error & exit
void cancel(int signo);

/*
 * Usage: printer-uri job-id user title copies options [file]
 */
int main(int argc, char *argv[], char *envp[])
{
   const char *devuri;         // printer's device URI
   int copies;                 // num of copies to print
   http_addrlist_t *addrlist;  // list of resolved addrs
   http_addrlist_t *connaddr;  // the chosen address
   char buf[256];              // general-purpose buffer
   int i;                      // all-purpose counter

   /*
    * Preparation:
    * Make sure that status messages are shipped out unbuffered!
    * Ignore SIGPIPE (we still get errno = EPIPE); see also
    *  http://www.developerweb.net/forum/showthread.php?t=2953
    * Catch common signals to terminate in an orderly fashion.
    */

   setbuf(stderr, NULL);

   if (signal(SIGPIPE, SIG_IGN) < 0)
      die(CUPS_BACKEND_FAILED, "signal");
   if (signal(SIGINT, cancel) < 0)
      die(CUPS_BACKEND_FAILED, "signal");
   if (signal(SIGQUIT, cancel) < 0)
      die(CUPS_BACKEND_FAILED, "signal");
   if (signal(SIGTERM, cancel) < 0)
      die(CUPS_BACKEND_FAILED, "signal");

   /*
    * Device discovery:
    * If invoked with no arguments, backends should list
    * all supported/detected printer devices to stdout or
    * at least tell what types of printers are supported.
    * We currently only do the latter...
    */

   if (argc == 1) {
      char *s = strrchr(argv[0], '/');
      if (s == NULL) s = argv[0]; else ++s;
      printf("network %s \"Unknown\" \"AppSocket/JetDirect w/Acct\"\n", s);
      return CUPS_BACKEND_OK;
   }

   /*
    * Backends are invoked with 6 or 7 arguments:
    * The optional 7th argument is the file to print and defaults
    * to stdin. Complain if not 6 or 7 arguments were supplied.
    */

   switch (argc) {
   case 6: // filtered printing
      jobfd = 0; // print stdin
      copies = 1; // cannot repeat
      break;
   case 7: // raw printing
      if ((jobfd = open(argv[6], O_RDONLY)) < 0)
         die(CUPS_BACKEND_FAILED, "open %s", argv[6]);
      copies = atoi(argv[4]);
      break;
   default:
      fprintf(stderr,
         "Usage: %s job-id user title copies options [file]\n", argv[0]);
      return CUPS_BACKEND_FAILED;
   }

   jobid = atoi(argv[1]);
   copyuser(jobuser, sizeof(jobuser), argv[2]);
   copytitle(jobtitle, sizeof(jobtitle), argv[3]);

   printer = getenv("PRINTER");
   if (printer == NULL) printer = hostname;

   /*
    * Parse the device URI into its constituents and
    * store them in global variables for easy access.
    *
    * Note that cupsBackendDeviceURI() is essentially an abbreviation
    * for this: (devuri = getenv("DEVICE_URI") ? devuri : argv[0])
    */

   devuri = cupsBackendDeviceURI(argv);
   if (devuri) parseURI(devuri);
   else {
      errno = 0;
      log_error("no device URI specified!");
      return CUPS_BACKEND_STOP;
   }

   /*
    * The pracc backend is now ready to do its actual work:
    * check credits, send the print job, and do accounting.
    */

   log_debug("This is cupspracc, version %s", VERSION);
   log_debug("Job %d user=%s title=%s", jobid, jobuser, jobtitle);
   log_debug("DeviceURI: %s", devuri);
   log_debug("Parameter: acctmode=%d pagecost=%d wait0=%d wait1=%d",
      acctmode, pagecost, wait0, wait1);
   log_debug("Jobscan: [%s]", jobscan ? jobscan : "(null)");
   log_debug("Running as: uid=%d euid=%d gid=%d egid=%d pid=%d",
      getuid(), geteuid(), getgid(), getegid(), getpid());

   /*
    * Run the optional job scanner and set jobpages accordingly.
    * For filtered jobs, copy stdin to a temporary file and then
    * work with that file, both for job scanning and printing.
    */

   if (jobscan && *jobscan) {
      if (jobfd == 0) {
         char fn[256];
         char buf[16384];
         int fd;
         ssize_t n;

         if ((fd = cupsTempFd(fn, sizeof(fn))) < 0)
            die(CUPS_BACKEND_FAILED, "cupsTempFd failed");
         if (unlink(fn) < 0)
            die(CUPS_BACKEND_FAILED, "unlink failed");

         log_debug("Copying stdin to %s", fn);
         do {
            n = read(jobfd, buf, sizeof(buf));
            if (n > 0) if (writen(fd, buf, n) < 0)
               die(CUPS_BACKEND_FAILED, "write %s failed");
         }
         while (n > 0);
         if (n < 0) die(CUPS_BACKEND_FAILED, "read failed");

         if (lseek(fd, 0, SEEK_SET) != 0) // rewind
            die(CUPS_BACKEND_FAILED, "lseek on temp file failed");

         jobfd = fd;
      }

      jobpages = runscan(jobfd, jobscan);
      if (lseek(jobfd, 0, SEEK_SET) != 0) // rewind
         die(CUPS_BACKEND_FAILED, "lseek on jobfd failed");
   }
   else jobpages = -1; // unknown

   /** Check credits **/

   if (acctmode) {
      long balance, limit;

      acctname = cupsGetJobBilling(printer, jobid);
      if (praccCheckName(acctname) < 0) acctname = jobuser;
      if (praccCheckName(acctname) < 0) // give up
         die(CUPS_BACKEND_CANCEL, "invalid job user name");

      log_debug("Check access: user %s to account %s", jobuser, acctname);
      checkaccess(jobuser, acctname); // die if access denied

      if (praccSum(acctname, &balance, &limit, 0, 0, 0) < 0) {
         if (errno == ENOENT) { // no such account
            if (praccSum(PRACCDEFLT, &balance, &limit, 0, 0, 0) < 0) {
               log_error("no account to bill (requested: %s)", acctname);
               return CUPS_BACKEND_FAILED;
            }
            else acctname = PRACCDEFLT;
         }
         else {
            log_error("error reading account %s", acctname);
            return CUPS_BACKEND_FAILED;
         }
      }

      /* Here: acctname & balance & limit are set */

      if (limit == UNLIMITED) strcpy(buf, "none");
      else sprintf(buf, "%ld", limit); // limited
      log_info("account %s: balance %ld limit %s", acctname, balance, buf);
      if (jobpages > 0) balance -= jobpages * pagecost;
      if ((balance < limit) && (limit != UNLIMITED)) {
         log_error("account %s: insufficient funds", acctname);
         return CUPS_BACKEND_OK; // XXX _CANCEL ?
      }
   }

   /** Connect **/

   addrlist = httpAddrGetList(hostname, AF_UNSPEC, portname);
   if (addrlist == NULL) {
      log_error("Cannot resolve %s", hostname);
      return CUPS_BACKEND_STOP; // stop queue!
   }

   log_debug("connecting to %s port %s...", hostname, portname);
   log_state("+connecting-to-device");

   while (1) {
      static int delay = 0;
      connaddr = httpAddrConnect(addrlist, &devfd);
      if (connaddr == NULL) {
         int saverr = errno;
         devfd = -1;

         /*
          * If the CLASS env var is set, the job was submitted
          * to a class and not to a specific printer. In this
          * case, terminate immediately so that the job can be
          * re-queued on the next available printer in the class.
          */

         if (getenv("CLASS")) {
            errno = (saverr) ? saverr : ECONNABORTED;
            log_info("%s port %s: %s", hostname, portname, strerror(errno));
            sleep(5); // do not re-queue too rapidly...
            return CUPS_BACKEND_FAILED;
         }

         /*
          * Report the error, sleep some seconds, and try again.
          * CUPS backends are required to do this forever...
          *
          * Better:
          * Check what the error is and give up immediately
          * if it is a permanent error, otherwise repeat up
          * to a max number of retries / connection timeout.
          */

         errno = (saverr) ? saverr : ECONNABORTED;
         log_error("%s port %s", hostname, portname);

         if (delay < 60) delay += 5;
         log_info("retrying in %d seconds", delay);
         sleep(delay);
      }
      else break; // connected
   }

   log_state("-connecting-to-device");

#ifdef AF_INET6
   if (connaddr->addr.addr.sa_family == AF_INET6)
      log_debug("connected to [%s]:%d (IPv6)",
         httpAddrString(&connaddr->addr, buf, sizeof(buf)),
         ntohs(connaddr->addr.ipv6.sin6_port));
   else
#endif // AF_INET6
      if (connaddr->addr.addr.sa_family == AF_INET)
         log_debug("connected to %s:%d (IPv4)",
            httpAddrString(&connaddr->addr, buf, sizeof(buf)),
            ntohs(connaddr->addr.ipv4.sin_port));

   /*
    * Finally, ignore SIGTERM to ensure the driver can emit
    * a complete page. For raw printing (no filter, no driver,
    * just the backend) do NOT ignore SIGTERM for otherwise
    * there is no way to cancel a raw print job.
    */

   if (jobfd == 0) // stdin means filtered printing
      signal(SIGTERM, SIG_IGN);

   /** Synchronise **/

   srand(time(0) ^ getpid());
   cookie = 10000 + (rand() & 65535);
  
   if (acctmode == POSTSCRIPT) {
      psinit();
      if (pscount(devfd, cookie))
         die(CUPS_BACKEND_FAILED, "pscount");
      pcpending = 0;
   }

   if (acctmode == PJL) {
      pjlinit();
      if (pjluel(devfd)) // just an UEL
         die(CUPS_BACKEND_FAILED, "pjluel");
      if (pjlecho(devfd, cookie)) // @PJL ECHO cookie
         die(CUPS_BACKEND_FAILED, "pjlecho");
      if (pjlcount(devfd)) // @PJL INFO PAGECOUNT
         die(CUPS_BACKEND_FAILED, "pjlcount");
      if (pjljob(devfd, jobid, 0, 0)) // @PJL JOB ...  (TODO cookie i/o jobid)
         die(CUPS_BACKEND_FAILED, "pjljob");
      pjlstate = PJLINIT;
   }

   /** Send print job **/

   log_info("Sending job data...");

   for (i = 0; i < copies; i++) {
      long nbytes;

      if (jobfd != 0) // raw printing (not stdin)
         lseek(jobfd, 0, SEEK_SET); // rewind

      nbytes = sendjob(jobfd, devfd);

      if (jobfd != 0 && nbytes >= 0)
         log_debug("Sent job file: %ld bytes, copy %d/%d", nbytes, i+1, copies);
   }

   /** Get job status **/

   if ((acctmode == PJL) || (acctmode == POSTSCRIPT)) {
      long lastpages;
      struct timeval timeout;

      log_info("Waiting for printer to finish...");

      if (acctmode == POSTSCRIPT) {
         for (i = wait0; i > 0; i = sleep(i));
         if (pscount(devfd, cookie) < 0)
            die(CUPS_BACKEND_FAILED, "pscount");
         lastpages = pages-1; // -2
         timeout.tv_sec = wait1;
      }
      else if (acctmode == PJL) {
         if (pjleoj(devfd, jobid)) // todo cookie instead of jobid
            die(CUPS_BACKEND_FAILED, "pjleoj");
         timeout.tv_sec = wait0;
      }

      while (1) {
         fd_set rfds;
         char buf[1024];
         int r;

         if (fdnonblock(devfd) < 0) // non-blocking printer i/o
            die(CUPS_BACKEND_STOP, "cannot set O_NONBLOCK on devfd");

         FD_ZERO(&rfds);
         FD_SET(devfd, &rfds);
         timeout.tv_usec = 0;

         if ((r = select(devfd+1, &rfds, NULL, NULL, &timeout)) > 0) {
            ssize_t n = read(devfd, buf, sizeof(buf));
            if (n > 0) parseinput(buf, n);
            else {
               if (n == 0) log_debug("Printer closed connection"); // EOF
               else log_debug("Reading printer failed: %s", strerror(errno));
               break; // nothing more to wait for...
            }
         }
         else {
            if (r == 0)
               log_debug("Timeout waiting for printer");
            break; // error or timeout: silently give up
         }

         if (acctmode == POSTSCRIPT) {
            if (pcpending && (pages == lastpages)) break;
            if (pages > 0) log_info("Pages printed: %d", pages);
            for (i = wait1; i > 0; i = sleep(i));
            if (pscount(devfd, cookie) < 0)
               die(CUPS_BACKEND_FAILED, "pscount");
            lastpages = pages;
            pcpending = 0;
            timeout.tv_sec = wait1;
         }
         else if (acctmode == PJL) {
            if (pjlstate == PJLDONE) break;
            timeout.tv_sec = wait1;
         }
      } // while
   } // if

   if (acctmode == PJL) {
      // Errors, typically broken pipe, are non-fatal here
      // because the printer might closed the connection by now.
      if (pjloff(devfd)) log_error("pjloff");
      if (pjluel(devfd)) log_error("pjluel");
   }

   close(devfd);
   connaddr = 0;
   httpAddrFreeList(addrlist);
   if (jobfd != 0) close(jobfd);

   /** Append Accounting Record **/

   if (acctmode) {
      int type;
      char info[96];
      long count, amount;

      if (jobpages > 0) jobpages *= copies;
      count = estimate(jobpages, pages);

      if (count < 0) {
         type = '!'; // error: pages unknown
         amount = 0;
      }
      else {
         type = '-'; // debit pages printed
         amount = count * pagecost;
      }

      acctstr(info, sizeof info);

      log_info("account %s: %c %s", acctname, type, info);
      if (praccAppend(acctname, type, amount, jobuser, info) != 0) {
         log_error("praccAppend %s failed", acctname);
         return CUPS_BACKEND_STOP; // stop queue!
      }
   }

   /** Append Pagecount Record **/

   switch (logpc(pagecount, printer)) {
      case 0: /* success */ break;
      case 1: /* no pc log */ break;
      default: log_error("logpc failed"); break;
   }

   log_info("Job done");
   return CUPS_BACKEND_OK;
}

/*
 * Send print job to printer and handle data sent back
 * by printer. This is essentially a select()-loop and
 * modelled after the CUPS backend/runloop.c source file.
 */
long sendjob(int jobfd, int devfd)
{
   int nfds;
   fd_set rfds, wfds;
   char buffer[8192];
   char *bufptr = buffer;
   long bytes = 0, total = 0;
   int offline = 0, nopaper = 0;

   if (fdnonblock(devfd) < 0) // want non-blocking printer i/o
      die(CUPS_BACKEND_STOP, "cannot set O_NONBLOCK on devfd");
   if (fdnonblock(jobfd) < 0) // want non-blocking job reading
      die(CUPS_BACKEND_STOP, "cannot set O_NONBLOCK on jobfd");

   nfds = 1 + ((jobfd > devfd) ? jobfd : devfd);

   while (1) {
      FD_ZERO(&rfds);
      if (bytes == 0)
         FD_SET(jobfd, &rfds);
      FD_SET(devfd, &rfds);

      FD_ZERO(&wfds);
      if (bytes > 0)
         FD_SET(devfd, &wfds);
      
      if (select(nfds, &rfds, &wfds, NULL, NULL) < 0) {
         log_debug("sendjob: select failed: errno=%d", errno);
         /* XXX unsure - taken from backend/runloop.c */
         if (errno == ENXIO && !offline) {
            log_state("+offline-error");
            log_info("Printer is currently offline");
            offline = 1;
         }
         sleep(2);
         continue;
      }

      /* Handle async printer input */

      if (FD_ISSET(devfd, &rfds)) {
         char mbuf[1024];
         ssize_t n = read(devfd, mbuf, sizeof(mbuf));
         if (n > 0) parseinput(mbuf, (int) n);
         else {
            if (n == 0) log_debug("Printer closed connection"); // EOF
            else log_debug("Reading printer failed: %s", strerror(errno));
            /* Otherwise ignore these errors */
         }
      }

      /* Read print job data */

      if ((bytes == 0) && FD_ISSET(jobfd, &rfds)) {
         ssize_t n = read(jobfd, buffer, sizeof(buffer));
         if (n > 0) bytes = n;
         else if (n < 0) {
            if (errno == EAGAIN || errno == EINTR)
               bytes = 0;
            else { // all other errors
               log_error("cannot read print job data");
               return -1; // see errno
            }
         }
         else break; // end-of-file, we're done

         bufptr = buffer; // reset bufptr
      }

      /* Write job data to printer */

      if ((bytes > 0) && FD_ISSET(devfd, &wfds)) {
         ssize_t n = write(devfd, bufptr, bytes);
         if (n < 0) switch (errno) {
            case ENOSPC:
               if (nopaper) break;
               log_error("Out of paper!");
               log_state("+media-empty-error");
               nopaper = 1;
               break;
            case ENXIO:
               if (offline) break;
               log_state("+offline-error");
               log_info("Printer is offline");
               offline = 1;
               break;
            case EAGAIN:
            case EINTR:
            case ENOTTY:
               break;
            default:
               log_error("cannot write printer: %s", strerror(errno));
               return -1;
         }
         else {
            if (nopaper) {
               log_state("-media-empty-error");
               nopaper = 0;
            }
            if (offline) {
               log_state("-offline-error");
               log_info("Printer now online");
               offline = 0;
            }

            bytes -= n;
            bufptr += n;
            total += n;
         }
      }
   }

   return total; // #bytes sent to printer
}

/*
 * Scan the print job on the given fd using the given
 * program. This program must read print job data from
 * its standard input and write the number of pages the
 * job would produce when printed to its standard output.
 * Also, it must return status 0 if successful.
 *
 * I don't use popen/pclose so I get a chance to dup
 * the given job fd to stdin between fork and exec.
 */
long runscan(int fd, const char *scanprog)
{
   FILE *scanner;
   char line[256];
   long num = -1;
   int pfd[2], n;
   pid_t pid;
   int status;

   assert(scanprog);

   log_debug("runscan(%d, %s)...", fd, scanprog);

   if (pipe(pfd) < 0)
      die(CUPS_BACKEND_FAILED, "pipe failed");

   if ((pid = fork()) < 0)
      die(CUPS_BACKEND_FAILED, "fork failed");

   if (pid == 0) { // child
      close(pfd[0]);
      if (pfd[1] != 1) {
         dup2(pfd[1], 1);
         close(pfd[1]);
      }
      dup2(fd, 0);
      close(3);
      close(4);
      execl(SHELL, "sh", "-c", scanprog, (char *) 0);
      _exit(127);
   }

   close(pfd[1]);
   scanner = fdopen(pfd[0], "r");
   if (!scanner)
      die(CUPS_BACKEND_FAILED, "fdopen failed");

   do {
      n = getln(scanner, line, sizeof(line), 0);
      if ((num < 0) && (n > 0)) {
         const char *p = line;
         line[n-1] = '\0'; // overwrite \n
         log_debug("Job scanner said: %s$", p);
         while (isspace(*p)) ++p;
         scanu(p, &num);
      }
   }
   while (n > 0);

   if (fclose(scanner) != 0)
      die(CUPS_BACKEND_FAILED, "fclose failed");
   while (waitpid(pid, &status, 0) < 0)
      if (errno != EINTR)
         die(CUPS_BACKEND_FAILED, "waitpid failed");
   if (status != 0)
      die(CUPS_BACKEND_FAILED, "%s failed", scanprog);

   return num; // jobpages, -1 if unknown
}

/*
 * Handle input from the printer:
 *
 * Look for messages (PostScript or PJL, depending on
 * the acctmode), parse messages by calling low-level
 * routines, and react upon the parsed message code.
 */
void parseinput(const char *buf, unsigned len)
{
   char out[64];
   int i, n;

   n = sizeof(out) - 1;
   if (len < n) n = len;
   for (i = 0; i < n; i++) {
      register char c = buf[i];
      if ((c == ' ') || isgraph(c)) out[i] = c;
      else out[i] = '.';
   }
   out[i] = '\0';

   log_debug("Got %d bytes from printer:", len);
   log_debug("%s", out);

   cupsBackChannelWrite(buf, len, 1.0);

   switch (acctmode) {
   case POSTSCRIPT:
      psinput(buf, len);
      break;
   case PJL:
      pjlinput(buf, len);
      break;
   default: /* ignore */ ;
   }
}

void psinput(const char *buf, unsigned len)
{
   register const char *p = buf;
   const char *end = buf + len;

   while (p < end) {
      int t = pschar(*p++);
      if (t) switch (t) {
      case PS_MSG_PAGECOUNT:
         if (ps_cookie == cookie) {
            log_debug("psinput: PAGECOUNT %d", ps_pagecount);
            if (pagecount < 0)
               pagecount = ps_pagecount; // remember initial pc
            else if (ps_pagecount >= pagecount)
               pages = ps_pagecount - pagecount;
            pcpending = 1; // got valid pc message
         }
         else log_debug("psinput: got cookie %ld, expected %ld",
                    ps_cookie, cookie);
         break;
      case PS_MSG_PRERROR:
         log_error("PrinterError: %s", ps_error);
         exit(CUPS_BACKEND_FAILED);
         break;
      case PS_MSG_FLUSHING:
         log_error("Flushing: rest of job will be ignored");
         exit(CUPS_BACKEND_CANCEL); // cancel job
         break;
      }
   }
}

/*
 * Parse PJL messages and try counting pages printed.
 * Use the messages to advance in a simple state diagram:
 *
 *   INIT---(1)-->SYNCED---(2)-->INJOB---(3)-->DONE
 *
 * Transitions: (1) ECHO with our cookie; (2) JOB with
 * our cookie; (3) EOJ with our cookie.
 *
 * While INJOB, use all PAGE and EOJ messages to set
 * the global pages variable, not just our EOJ message.
 * Reason: at least the HP LaserJet 5000 gets the EOJ
 * pages wrong for nested JOB/EOJ pairs; to reproduce,
 * print a Windows Test Page to an HP LaserJet 5000...
 */
void pjlinput(const char *buf, unsigned len)
{
   register const char *p = buf;
   const char *end = buf + len;

   while (p < end) {
      int msg = pjlchar(*p++);
      if (msg) switch (msg) {
      case PJL_MSG_COOKIE:
         if (pjl_cookie != cookie) break; // not our cookie
         log_debug("pjlinput: state=%d COOKIE %ld (ok)",
                   pjlstate, pjl_cookie);
         if (pjlstate != PJLINIT) break; // ignore unexpected ECHO message
         pjlstate = PJLSYNC;
         break;
      case PJL_MSG_PAGECOUNT:
         if (pjlstate != PJLSYNC) break;
         log_debug("pjlinput: state=%d PAGECOUNT %ld",
                   pjlstate, pjl_pagecount);
         if (pagecount < 0) pagecount = pjl_pagecount; // only once
         break;
      case PJL_MSG_JOBSTART:
         log_debug("pjlinput: state=%d JOB %ld",
                   pjlstate, pjl_jobnum);
         if (pjlstate != PJLSYNC) break; // out-of-order
         if (pjl_jobnum != jobid) break; // not our job (todo cookie i/o jobid)
         pjlstate = PJLJOB;
         break;
      case PJL_MSG_JOBEND:
         log_debug("pjlinput: state=%d EOJ %ld %ld",
                   pjlstate, pjl_jobnum, pjl_numpages);
         if (pjlstate != PJLJOB) break;
         if (pjl_numpages > pages) pages = pjl_numpages;
         if (pjl_jobnum == jobid) { // todo cookie instead of jobid
            pjlstate = PJLDONE;
         }
         break;
      case PJL_MSG_PAGE:
         log_debug("pjlinput: state=%d PAGE %ld",
                   pjlstate, pjl_curpage);
         if (pjlstate != PJLJOB) break; // ignore outside "our" job
         if (pjl_curpage > pages) pages = pjl_curpage;
         if (jobfd != 0) log_page(pages, 1);
         log_info("Printed page %d", pjl_curpage);
         break;
      }
   }
}

/*
 * Estimate the number of pages printed, based on the number
 * of pages in the printjob (-1 if unknown) and the number of
 * pages printed as reported by the printer (-1 if unknown).
 *
 * In theory, counting with PJL is much more robust than counting
 * with PostScript, therefore, if acctmode is PJL, it's reasonable
 * to trust n and to not average it with m. In practice, however,
 * there are drivers that emit invalid PJL code and the count is
 * completely wrong!
 */
long estimate(long jobcount, long devcount)
{
   if (jobcount < 0 && devcount < 0) return -1; // both unknown

   if (jobcount < 0) return devcount;
   if (devcount < 0) return jobcount;

   //if (acctmode == PJL) return devcount; TODO DROP (see comment above)

   if (jobcount < devcount) return devcount;

   return (jobcount+devcount)/2;
}

/*
 * Build the accounting info string in the given buffer,
 * using information from global variables. The buffer
 * should be at least 90 bytes. Account info string format:
 *
 *   print JID PRINTER JOBPAGES PAGES [JOBTITLE]
 *
 * JOBPAGES and PAGES are -1 if unknown.
 */
void acctstr(char *buf, int maxlen)
{
   register char *p = buf;
   p += prints(p, "print ");
   p += printu(p, jobid);
   p += printc(p, ' ');
   p += printsn(p, printer, 30);
   p += printc(p, ' ');
   p += printi(p, jobpages); // -1 if unknown
   p += printc(p, ' ');
   p += printi(p, pages); // -1 if unknown
   if (jobtitle && jobtitle[0]) {
      p += printc(p, ' ');
      p += printsn(p, jobtitle, 30);
   }
   p += print0(p);
}

/*
 * Append a record to the pagecount log file.
 *
 * The format of pagecount log records is
 *
 *   @timestamp pagecount printer [comment]
 *
 * Return 0 if ok, 1 if no pagecount log file,
 * and -1 on system errors (see errno).
 */
int logpc(long pc, const char *printer)
{
   struct tai now;
   char buf[80];
   char *p, *endp;
   int fd, len;

   fd = open(PRACCPCLOG, O_WRONLY | O_APPEND);
   if (fd < 0) {
      if (errno == ENOENT) return 1; // no pc log
      else return -1; // general error, see errno
   }

   tainow(&now); // system time

   p = buf;
   endp = buf + sizeof buf - 1;
   p += taifmt(p, &now);
   p += printc(p, ' ');
   p += printi(p, pc);
   p += printc(p, ' ');
   if (!printer) printer = "unknown";
   p += printsn(p, printer, endp-p);
   p += printc(p, '\n');

   len = p - buf;
   if (write(fd, buf, len) < 0) return -1; // see errno
   if (close(fd) < 0) return -1; // see errno

   return 0; // ok
}

/*
 * Parse the device URI into its constituents and
 * store them in global variables for easy access.
 */
void parseURI(const char *deviceURI)
{
   char method[256];
   char resource[2048];
   char *options;
   http_uri_status_t result;
   const char *name, *value;
   long number;
   int portnum = 0;

   result = httpSeparateURI(HTTP_URI_CODING_ALL, deviceURI,
      method, sizeof(method), username, sizeof(username),
      hostname, sizeof(hostname), &portnum,
      resource, sizeof(resource));

   if (portnum == 0) portnum = 9100; // default: JetDirect
   portname[printu(portname, portnum)] = '\0';

   if ((options = strchr(resource, '?')) != NULL)
   {
      *options++ = '\0';
      while (*options)
      {
         name = options;
         while (*options && *options != '=' && *options != '&') ++options;
         value = options++;
         if (*value == '=') {
            *(char*)value++ = '\0'; // overwrite '=' with NUL
            while (*options && *options != '&' && *options != '+')
               ++options;
            if (*options) *options++ = '\0';
         }
         else *(char*)value = '\0'; // empty string

         /* Process the name=value pair */
         if (!strcasecmp(name, "acct")) {
            acctmode = parseMode(value, NONE);
            if (wait0 < 0) switch (acctmode) {
               case POSTSCRIPT: wait0 = DEFLT_WAIT0_PS; break;
               case PJL: wait0 = DEFLT_WAIT0_PJL; break;
               default: wait0 = 0; break;
            }
            if (wait1 < 0) switch (acctmode) {
               case POSTSCRIPT: wait1 = DEFLT_WAIT1_PS; break;
               case PJL: wait1 = DEFLT_WAIT1_PJL; break;
               default: wait1 = 0; break;
            }
         }
         else if (!strcasecmp(name, "pagecost")) {
            pagecost = atoi(value);
         }
         else if (!strcasecmp(name, "wait0")) {
            if (scanu(value, &number))
               wait0 = (int) number;
         }
         else if (!strcasecmp(name, "wait1")) {
            if (scanu(value, &number))
               wait1 = (int) number;
         }
         else if (!strcasecmp(name, "jobscan")) {
            jobscan = strdup(value); // copy!
         }
      }
   }
}

/*
 * Translate the string pointed to by value
 * to one of the accounting/synchronising modes.
 */
mode parseMode(const char *value, mode deflt)
{
   if (strcasecmp(value, "off") == 0) return NONE;
   if (strcasecmp(value, "PJL") == 0) return PJL;
   if (strcasecmp(value, "PS") == 0) return POSTSCRIPT;
   if (strcasecmp(value, "PostScript") == 0) return POSTSCRIPT;
   if (strcasecmp(value, "job") == 0) return JOBSCAN;
   return deflt;
}

/*
 * Translate the string pointed to by value to either
 * true (1) or false (0), or return the given default.
 */
int parseOnOff(const char *value, int deflt)
{
   if (strcasecmp(value, "on") == 0) return 1;
   if (strcasecmp(value, "off") == 0) return 0;
   if (strcasecmp(value, "yes") == 0) return 1;
   if (strcasecmp(value, "no") == 0) return 0;
   if (strcasecmp(value, "true") == 0) return 1;
   if (strcasecmp(value, "false") == 0) return 0;
   return deflt;
}

int copyuser(char *dest, int size, const char *s)
{
   int i = 0;

   while (i < size-1)
   {
      char c = s[i];
      if (!c) break;

      dest[i] = tolower(c);
      i += 1;
   }

   dest[i] = 0; // terminate string
}

/*
 * Copy the job title given in s into the buffer dest[size].
 * Chop an initial "smbprn.XXXXXXXX", replace white space
 * by underscores, and replace non-printable characters by
 * question marks.
 *
 * The resulting string in dest is always NUL-terminated.
 * Return the number of characters copied.
 */
int copytitle(char *dest, int size, const char *s)
{
   register char *p = dest;
   char *end = p + size - 1;

   if (strncmp("smbprn.", s, 7) == 0) {
      s += 7; while (*s)
         if (!isdigit(*s++)) break;
   }

   while (p < end) {
      register char c = *s++;
      if (isgraph(c)) *p++ = c;
      else if (isspace(c)) *p++ = '_';
      else if (c != '\0') *p++ = '?';
      else break; // end of string
   }
   if (size > 0) *p = '\0';

   return p - dest; // #chars
}

/*
 * Check if user 'username' is allowed to access account
 * 'acctname' and exit if not.
 *
 * If access is denied, try to record the error in the
 * requesting user's personal account (this is useful
 * feedback on invalid job-billing values), then quit
 * and cancel the job.
 */
void checkaccess(const char *username, const char *acctname)
{
   int result;

   assert(username && acctname);

   result = praccGrant(username, acctname);
   if (result < 0) // system error - see errno
      die(CUPS_BACKEND_FAILED, "praccGrant failed");

   if (result > 0) { // deny access to account
      char s[MAXNAME+32];

      strncpy(s, acctname, MAXNAME);
      strcat(s, ": Access denied");
      praccAppend(username, '!', -1, jobuser, s);

      die(CUPS_BACKEND_CANCEL, "praccGrant(%s to %s) = DENY",
          username, acctname);
   }

   log_debug("praccGrant(%s to %s) = GRANT", username, acctname);
}

/*
 * Write the given buffer to the given file descriptor
 * in blocking mode: do not return until all data has
 * been written or an error occurred.
 *
 * Use this function for sending small amounts of data
 * to the printer en block without bothering about the
 * printer's asynchronous replies. Careful: this could
 * create a dead-lock (no select-loop)!
 *
 * Return 0 if OK or -1/errno if an error occurred.
 */
int writeall(int fd, const char *buf, unsigned len)
{
   int ret;

   if (fdblocking(fd) < 0) return -1; // see errno

   if (writen(fd, buf, len) < 0) return -1; // see errno

   return 0; // OK
}

/*
 * On some special devices (notably terminals, networks, streams)
 * a write() operation may return less than specified. This isn't
 * an error and we should continue with the remainder of the data.
 * This phenomenon never happens with ordinary disk files.
 *
 * See Stevens (1993, p.406-408) for details.
 */
ssize_t writen(int fd, const void *buf, size_t len)
{
   size_t nbytes = len;
   char * bufptr = (char *) buf;  // no ptr arith with void star

   while (nbytes > 0) {
      ssize_t n = write(fd, bufptr, nbytes);
      if (n <= 0) return -1; // see errno
      nbytes -= n;
      bufptr += n;
   }

   return len;
}

/*
 * These two functions take a descriptor of an open file (or pipe,
 * socket, etc) and set/clear the O_NONBLOCK bit in the status flags
 * Return -1 on error, some other value if ok.
 */

#ifndef O_NONBLOCK
#error Your system headers do not define O_NONBLOCK.
#endif

int fdblocking(int fd)
{
   return fcntl(fd, F_SETFL, fcntl(fd, F_GETFL, 0) & ~O_NONBLOCK);
}

int fdnonblock(int fd)
{
   return fcntl(fd, F_SETFL, fcntl(fd, F_GETFL, 0) | O_NONBLOCK);
}

/*
 * Try getting the job-billing IPP/CUPS job attribute.
 * Return an malloc'ed string of NULL on failure.
 *
 * The Get-Job-Attributes operation request attributes:
 *  attributes-charset
 *  attributes-natural-language
 *  printer-uri AND job-id OR job-uri
 *
 * This function makes use of CUPS API routines.
 */
char *cupsGetJobBilling(const char *printer, long jobid)
{
   http_t *http;          // HTTP connection object
   ipp_t *request;        // IPP request object
   ipp_t *response;       // IPP response object
   ipp_attribute_t *attr; // current IPP attr in response
   const char *const attrs[] = { "job-billing" };
   int numattrs = 1; // sizeof(attrs) / sizeof(attrs[0])
   char uri[HTTP_MAX_URI];
   char *jobBillingString;

   http = httpConnectEncrypt(cupsServer(), ippPort(), cupsEncryption());
   if (!http) return (char *) 0;

   request = ippNewRequest(IPP_GET_JOB_ATTRIBUTES);
   if (!request) return (char *) 0;

   snprintf(uri, sizeof uri, "ipp://%s/printers/%s/",
      cupsServer(), printer);
   ippAddString(request, IPP_TAG_OPERATION, IPP_TAG_URI,
      "printer-uri", NULL, uri);

   ippAddInteger(request, IPP_TAG_OPERATION, IPP_TAG_INTEGER,
      "job-id", jobid);

   numattrs = sizeof(attrs) / sizeof(attrs[0]);
   ippAddStrings(request, IPP_TAG_OPERATION, IPP_TAG_KEYWORD,
      "requested-attributes", numattrs, NULL, attrs);

   jobBillingString = (char *) 0; // assume failure
   response = cupsDoRequest(http, request, "/"); // XXX /jobs ?
   if (response && response->request.status.status_code == IPP_OK)
      for (attr = response->attrs; attr; attr = attr-> next)
         if (strcmp(attr->name, "job-billing") == 0)
            jobBillingString = strdup(attr->values[0].string.text);

   ippDelete(response); // request is deleted by cupsDoRequest()
   httpClose(http);

   return jobBillingString;
}

static void private_log(const char *fmt, ...)
{
   static FILE *logfp = 0;
   static int first = 1;
   va_list ap;

   if (first) {
      int logfd;
      struct stat stbuf;

      first = 0;

      /* Can't use fopen() because it creates the file! */
      logfd = open(PRIVATE_LOGFILE, O_APPEND | O_WRONLY);
      if (logfd < 0) return; // silently give up
      logfp = fdopen(logfd, "a");
      if (!logfp) return; // silently give up

#if 0 // Version 1.2.0 and later: don't truncate
      /* Truncate to zero if too long */
      if (fstat(fileno(logfp), &stbuf) == 0)
         if (stbuf.st_size > 1000000)
            ftruncate(fileno(logfp), 0);
#endif
   }

   if (logfp) {
      va_start(ap, fmt);
      fprintf(logfp, "Job %ld: ", jobid); // tag
      vfprintf(logfp, fmt, ap);
      fflush(logfp);
      va_end(ap);
   }
}

/*
 * Notify the CUPS log file about normal things going on.
 * Use warn() for warnings and die() for fatal errors.
 *
 * Note: The scheduler (at least in version 1.2.2) seems
 *       to NOT log INFO strings from backends. Annoying.
 */
void log_info(const char *fmt, ...)
{
   char msg[256];

   va_list ap;
   va_start(ap, fmt);

   vsnprintf(msg, sizeof msg, fmt, ap);
   fprintf(stderr, "INFO: %s\n", msg);
   private_log("INFO: %s\n", msg);

   va_end(ap);
}

/*
 * Issue a PAGE message to standard error. There are two types
 * of PAGE messages used by CUPS:
 *
 * PAGE: n m         # add m to job-media-sheets-completed attr
 * PAGE: n total     # set job-media-sheets-completed attr to n
 *
 * If copies is negative, then the second version is produced,
 * if both arguments are positive, the first version is written.
 */
void log_page(long number, long copies)
{
   assert(number >= 0);

   if (copies < 0) fprintf(stderr, "PAGE: %ld total\n", number);
   else fprintf(stderr, "PAGE: %ld %ld\n", number, copies);
}

/*
 * Issue a state change message to standard error:
 *
 * STATE: +printer-state-reason      # add
 * STATE: -printer-state-reason      # remove
 * STATE: printer-state-reason       # set
 *
 * The CUPS scheduler will parse these messages and add/remove/set
 * printer-state-reason keywords to the print queue. Typical use is
 * to report media and ink/toner conditions. Backends may also use
 * it to report "connecting-to-device" to the scheduler.
 */
void log_state(const char *s)
{
   fprintf(stderr, "STATE: %s\n", s);
}

/*
 * Issue a debug message to standard error
 * Because CUPS seems to loose messages,
 * we also log to our private debug log!
 */
void log_debug(const char *fmt, ...)
{
   char msg[256];

   va_list ap;
   va_start(ap, fmt);

   vsnprintf(msg, sizeof msg, fmt, ap);
   fprintf(stderr, "DEBUG: %s\n", msg);
   private_log("DEBUG: %s\n", msg);

   va_end(ap);
}

/*
 * Issue an error message to standard error.
 * Append strerror(errn) if errno is non-zero.
 */
void log_error(const char *fmt, ...)
{
   char msg[256];

   va_list ap;
   va_start(ap, fmt);

   vsnprintf(msg, sizeof msg, fmt, ap);
   if (errno == 0) fprintf(stderr, "ERROR: %s\n", msg);
   else fprintf(stderr, "ERROR: %s: %s\n", msg, strerror(errno));
   if (errno == 0) private_log("ERROR: %s\n", msg);
   else private_log("ERROR: %s: %s\n", msg, strerror(errno));

   va_end(ap);
}

/*
 * Issue an error message to standard error.
 * Append strerror(errno) if errno is non-zero.
 * Exit with given code (see CUPS_BACKEND_XXX constants).
 */
void die(int code, const char *fmt, ...)
{
   char msg[256];
   int saverr = errno;

   va_list ap;
   va_start(ap, fmt);

   vsnprintf(msg, sizeof(msg), fmt, ap);

   if (saverr == 0) fprintf(stderr, "ERROR: %s\n", msg);
   else fprintf(stderr, "ERROR: %s: %s\n", msg, strerror(errno));
   if (saverr == 0) private_log("ERROR: %s\n", msg);
   else private_log("ERROR: %s: %s\n", msg, strerror(errno));

   va_end(ap);

   exit(code);
}

void cancel(int signo)
{
   errno = 0; // prevent strerror(errno) in message!
   log_error("Killed by signal %d", signo);
   log_info("Job cancelled, printer ready");
   exit(CUPS_BACKEND_CANCEL);
}
