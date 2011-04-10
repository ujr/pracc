/* pjl.c - part of pracc CUPS backend */
/* Copyright (c) 2005-2007 by Urs-Jakob Rueetschi */
/* Published under the GNU General Public License */

static char \
rcsid[] = "$Id: pjl.c,v 1.3 2009/10/06 19:25:08 ujr Exp ujr $";

#include <ctype.h>
#include <stdio.h>
#include <string.h>

#include "pjl.h"

#define UEL "\033%-12345X"  // Universal Exit Language

extern int writeall(int fd, const char *buf, unsigned len);

static int pjlwrite(int fd, const char *buf, unsigned len);
static int scantok(const char *s, unsigned long *len, const char *delim);

long pjl_cookie = 0;
long pjl_pagecount = -1;
long pjl_jobnum = -1;
long pjl_numpages = -1;
long pjl_curpage = -1;

static enum {
   NORMAL,
   GOTAT, GOTATP, GOTATPJ, GOTATPJL,
   INMSG
} state = NORMAL;

static struct {
   char *ptr, *end;
   char buf[1024];
} msg = { msg.buf, msg.buf+1024 };

static void msgchar(char c)
{  /* Append char to message buffer */
   if (msg.ptr < msg.end) *msg.ptr++ = c;
   else /* buffer full, char silently lost */ ;
}

static const char *msgterm(void)
{  /* Add NUL, reset buffer, return str */
   if (msg.ptr < msg.end) *msg.ptr++ = '\0';
   else *(msg.end-1) = '\0'; // overwrite

   msg.ptr = msg.buf; // reset buffer
   return (const char *) msg.buf;
}

/*
 * Reset global variables to their default values.
 * The parser - pjlparse() - will change them in
 * accordance with the messages parsed.
 */
void pjlinit(void)
{
   pjl_cookie = 0;           // no cookie
   pjl_pagecount = -1;       // unknown
   pjl_jobnum = -1;          // unknown
   pjl_numpages = -1;        // unknown
   pjl_curpage = -1;         // unknown

   state = NORMAL;           // reset msg parser
   msg.ptr = msg.buf;        // reset msg buffer
}

/*
 * Send a standalone UEL sequence.
 *
 * Send:   UEL
 * Expect: (nothing)
 *
 * Return 0 if ok and -1/errno on failure.
 */
int pjluel(int fd)
{
#if 0
   char *s;
   char buf[20]; // XXX
   int n;

   s = "%s@PJL \r\n";
   n = snprintf(buf, sizeof buf, s, UEL);
#endif

   return pjlwrite(fd, UEL, strlen(UEL));
}

/*
 * Send a PJL ECHO statement that contains our cookie.
 * The calling software should then watch for PJL messages
 * and invoke pjlparse() to check if our cookie has arrived.
 *
 * Send:   UEL@PJL ECHO <cookie> <CR><LF>
 * Expect: @PJL ECHO <cookie> [<CR>]<LF> <FF>
 *
 * Return 0 if ok and -1/errno on failure.
 */
int pjlecho(int fd, int cookie)
{
   char *s;
   char buf[80]; // XXX
   int n;

   s = "@PJL ECHO %d \r\n";
   n = snprintf(buf, sizeof buf, s, cookie);

   return pjlwrite(fd, buf, n);
}

/*
 * Send a PJL INFO PAGECOUNT statement that asks the printer
 * to return the value of its pagecount register. Watch for
 * PJL messages and invoke pjlparse() to parse them.
 *
 * Send:   @PJL INFO PAGECOUNT <CR><LF>
 * Expect: @PJL INFO PAGECOUNT <NL> [PAGECOUNT [=]] <number> <NL> <FF>
 *
 * Return 0 if ok and -1/errno on failure.
 */
int pjlcount(int fd)
{
   char *s = "@PJL INFO PAGECOUNT \r\n";

   return pjlwrite(fd, s, strlen(s));
}

/*
 * Send a PJL JOB (beginning of print job) command to the printer.
 * The cookie is used as the job's name and the personality, if not
 * null, is used in a subsequent ENTER LANGUAGE command; typically,
 * personality is either "POSTSCRIPT" or "PCL".
 *
 * Send:   UEL@PJL <CR><LF>
 *         @PJL USTATUS JOB = ON <CR><LF>
 *         @PJL USTATUS PAGE = ON <CR><LF>
 *         @PJL JOB NAME = "jobid" <CR><LF>
 *         @PJL ENTER LANGUAGE = "personality" [or just a UEL]
 *
 * Expect: Wait for a USTATUS JOB message, which
 *         indicates that the job is now printing; 
 *
 * Return 0 if ok and -1/errno on failure.
 */
int pjljob(int fd, int jobid, char *personality, char *display)
{
   char *s;
   char buf[128]; // XXX
   char str[64]; // XXX
   int n;

   s = "%s@PJL \r\n"; // intro
   n = snprintf(buf, sizeof buf, s, UEL);
   if (pjlwrite(fd, buf, n) < 0) return -1;

   s = "@PJL USTATUS JOB = ON \r\n";
   if (pjlwrite(fd, s, strlen(s)) < 0) return -1;

   s = "@PJL USTATUS PAGE = ON \r\n";
   if (pjlwrite(fd, s, strlen(s)) < 0) return -1;

   s = "@PJL JOB NAME = \"%d\" DISPLAY = \"%s\" \r\n";
   if (display == 0) {
      snprintf(str, sizeof(str), "Job %d printing", jobid);
      display = str;
   }
   n = snprintf(buf, sizeof buf, s, jobid, display);
   if (pjlwrite(fd, buf, n) < 0) return -1;

   if (personality) {
      s = "@PJL ENTER LANGUAGE = \"%s\" \r\n";
      n = snprintf(buf, sizeof buf, s, personality);
      if (pjlwrite(fd, buf, n) < 0) return -1;
   }
   else if (pjlwrite(fd, UEL, strlen(UEL)) < 0) return -1;

   return 0; // SUCCESS
}

/*
 * Send a PJL EOJ (end-of-job) command to the printer.
 * This must follow a PJL JOB command and JOB/EOJ pairs
 * must be properly nested.
 *
 * Send:   UEL@PJL <CR><LF>
 *         @PJL EOJ NAME = "cookie" <CR><LF>
 *
 * Expect: Wait for a USTATUS JOB message, which
 *         indicates that the job finished printing.
 *
 * Return 0 if ok and -1/errno on failure.
 */
int pjleoj(int fd, int cookie)
{
   char *s;
   char buf[80]; // XXX
   int n;

   s = "%s@PJL \r\n"; // intro
   n = snprintf(buf, sizeof buf, s, UEL);
   if (pjlwrite(fd, buf, n) < 0) return -1;

   s = "@PJL EOJ NAME = \"%d\" \r\n";
   n = snprintf(buf, sizeof buf, s, cookie);
   if (pjlwrite(fd, buf, n) < 0) return -1;

   return 0; // SUCCESS
}

/*
 * Send the USTATUSOFF command, which turns off
 * all unsolicited (asynchronous) notifications.
 *
 * Send:   UEL@PJL USTATUSOFF <NL>
 * Expect: (nothing)
 *
 * Return 0 if ok and -1/errno on failure.
 */
int pjloff(int fd)
{
   char *pat = "%s@PJL USTATUSOFF\r\n";
   char buf[40]; // XXX

   int n = snprintf(buf, sizeof buf, pat, UEL);

   return pjlwrite(fd, buf, n);
}

/*
 * Write the given buffer to the given file descriptor
 * in blocking mode, that is, do not return until all
 * has been written or an error occurred.
 */
static int pjlwrite(int fd, const char *buf, unsigned len)
{
   return writeall(fd, buf, len);
}

/*
 * Parser for PJL messages. Should be invoked whenever
 * a complete message was received from the printer, with
 * the initial @PJL and the final <FF> stripped:
 *   @PJL (...) <FF>
 * This parser looks for these types of messages:
 *
 *   ECHO cookie <NL>
 *   INFO PAGECOUNT <NL> [PAGECOUNT [=]] <number> <NL>
 *   USTATUS JOB <NL> START <NL> NAME = "name" <NL>
 *   USTATUS JOB <NL> END <NL> NAME = "name" <NL> PAGES = <number> <NL>
 *   USTATUS PAGE <NL> <number> <NL>
 *
 * Here <NL> stands for [<CR>]<LF>, that is, a newline.
 * The parser returns one of the PJL_MSG_XXX constants
 * depending on the message parsed.
 *
 * The parser is quite liberal regarding message formats.
 */
int pjlparse(const char *msg)
{
   const char *tok;
   unsigned long n;
   unsigned long number;
   const char *p = msg;

   //fprintf(stderr, "{pjlparse:%s}\n", msg); //DEBUG

   while (isspace(*p)) p++;
   tok = p; p += scantok(p, &n, " \t\n\r\f\v\b");
   if (n == 0) return PJL_MSG_OTHER;

   if (strncmp(tok, "ECHO", n) == 0) {
      p += (n = scani(p, &number));
      while (isspace(*p)) ++p;
      if (n && (*p == '\0')) {
         pjl_cookie = number;
         return PJL_MSG_COOKIE;
      }
      return PJL_MSG_OTHER;
   }

   if (strncmp(tok, "INFO", n) == 0) {
      tok = p; p += scantok(p, &n, " \b\t\n\v\f\r");
      if (n == 0) return PJL_MSG_OTHER;
      if (strncmp(tok, "PAGECOUNT", n) == 0) {
         while (*p && !isdigit(*p)) ++p;
         pjl_pagecount = (scanu(p, &number)) ? number : -1;
         return PJL_MSG_PAGECOUNT;
      }
      return PJL_MSG_OTHER;
   }

   if (strncmp(tok, "USTATUS", n) == 0) {
      tok = p; p += scantok(p, &n, " \b\t\n\v\f\r");
      if (n == 0) return PJL_MSG_OTHER;
      if (strncmp(tok, "JOB", n) == 0) {
         enum { UNKNOWN, START, END } job = UNKNOWN;
         tok = p; p += scantok(p, &n, " \b\t\n\v\f\r");
         if (n == 0) return PJL_MSG_OTHER;
         if (strncmp(tok, "START", n) == 0) job = START;
         else if (strncmp(tok, "END", n) == 0) job = END;
         if (job == UNKNOWN) return PJL_MSG_OTHER;

         /* Now look for NAME = "jobnum" XXX */
         tok = p; p += scantok(p, &n, " =\"\b\t\n\v\f\r");
         if (n == 0) return PJL_MSG_OTHER;
         if (strncmp(tok, "NAME", n)) return PJL_MSG_OTHER;
         tok = p; p += scantok(p, &n, " \"\b\t\n\v\f\r");
         if (n == 0) return PJL_MSG_OTHER;
         pjl_jobnum = (scanu(tok, &number)) ? number : -1;
         if (job == START) return PJL_MSG_JOBSTART;

         /* Now look for PAGES = number XXX */
         tok = p; p += scantok(p, &n, " =\b\t\n\v\f\r");
         if (n == 0) return PJL_MSG_OTHER;
         if (strncmp(tok, "PAGES", n)) return PJL_MSG_OTHER;
         tok = p; p += scantok(p, &n, "\b\t\n\v\f\r");
         if (n == 0) return PJL_MSG_OTHER;
         pjl_numpages = (scanu(tok, &number)) ? number : -1;
         if (job == END) return PJL_MSG_JOBEND;

         return PJL_MSG_OTHER;
      }
      if (strncmp(tok, "PAGE", n) == 0) {
         while (*p && !isdigit(*p)) ++p;
         pjl_curpage = (scanu(p, &number)) ? number : -1;
         return PJL_MSG_PAGE;
      }
      return PJL_MSG_OTHER;
   }
   return PJL_MSG_OTHER;
}

/*
 * Tokenizer for PJL message parser.
 */
static
int scantok(const char *s, unsigned long *len, const char *delim)
{
   unsigned long n;
   const char *p = s;

   if (len) *len = 0;

   n = strcspn(p, delim);
   if (n == 0) return 0; // no token

   p += n;
   if (len) *len = n;
   return n + strspn(p, delim);
}

/*
 * This routine is a simple state machine looking for
 * PJL mesages. If one is found, it is submitted to
 * pjlparse() and its value returned, otherwise, zero
 * will be returned as an indication that there was
 * no message found so far.
 */
int pjlchar(char c)
{
   int found = 0;

   switch (state) { /* FSM */
      case NORMAL:
         if (c == '@') state = GOTAT;
         /* else keep state NORMAL */
         break;
      case INMSG:
         if (c == '\f') { found = 1; state = NORMAL; }
         else msgchar(c); // keep state INMSG
         break;
      case GOTAT:
         if (c == 'P') state = GOTATP;
         else state = NORMAL;
         break;
      case GOTATP:
         if (c == 'J') state = GOTATPJ;
         else state = NORMAL;
         break;
      case GOTATPJ:
         if (c == 'L') state = GOTATPJL;
         else state = NORMAL;
         break;
      case GOTATPJL:
         if (isspace(c)) state = INMSG;
         else state = NORMAL;
         break;
   }
   return (found) ? pjlparse(msgterm()) : 0;
}

#ifdef TESTING
#include <stdio.h>
#include <string.h>
int main(void)
{
   char buf[1024];

   pjlinit();
   printf("Type text including @PJL message text <FF>\n");
   printf("Type Ctrl-L to get <FF> and check if pjl.c\n");
   printf("Check if pjl.c correctly recognises/parses the messages\n");

   pjlinit();
   while (fgets(buf, sizeof buf, stdin)) {
      register char *p = buf;
      while (*p) switch (pjlchar(*p++)) {
      case PJL_MSG_COOKIE:
         printf("{COOKIE:pjl_cookie=%ld}", pjl_cookie);
         break;
      case PJL_MSG_PAGECOUNT:
         printf("{PAGECOUNT:pjl_pagecount=%ld}", pjl_pagecount);
         break;
      case PJL_MSG_JOBSTART:
         printf("{JOBSTART:pjl_jobnum=%ld}", pjl_jobnum);
         break;
      case PJL_MSG_JOBEND:
         printf("{JOBEND:pjl_jobnum=%ld,pjl_numpages=%ld}",
            pjl_jobnum, pjl_numpages);
         break;
      case PJL_MSG_PAGE:
         printf("{PAGE:pjl_curpage=%ld}", pjl_curpage);
         break;
      case PJL_MSG_OTHER:
         printf("{other}");
         break;
      default:
         putchar('.');
      }
      printf("\n");
      fflush(stdout);
   }
   return 123; // TESTING
}
int writeall(int fd, const char *buf, unsigned len)
{ return write(fd, buf, len) == len; }
#endif // TEST
