/* ps.c - part of pracc CUPS backend */
/* Copyright (c) 2004-2007 by Urs-Jakob Rueetschi */
/* Published under the GNU General Public License */

//static char rcsid[] = "$Id: ps.c,v 1.4 2008/01/04 09:37:18 ujr Exp ujr $";

#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ps.h"
#include "scan.h"

extern int writeall(int fd, const char *buf, unsigned len);

static int pswrite(int fd, const char *buf, unsigned len);
static int getnext(const char *s, char **tok, int *len, char stop);

long ps_cookie = 0;
long ps_pagecount = -1;
int ps_flushing = 0;
char ps_error[64] = "";

static enum {
   NORMAL,
   GOTPERC, GOTPERCPERC, INMSG,
   GOTBRACK, GOTBRACKPERC
} state = NORMAL;

static struct {
   char *ptr, *end;
   char buf[1024];
} msg = { msg.buf, msg.buf+1024, "" };

static void msgchar(char c)
{  /* Append char to message buffer */
   if (msg.ptr < msg.end) *msg.ptr++ = c;
   else { /* buffer full, char silently lost */ }
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
 * The parser - psparse() - will change them in
 * accordance with the messages parsed.
 */
void psinit(void)
{
   ps_cookie = 0;            // no cookie
   ps_pagecount = -1;        // unknown
   ps_flushing = 0;          // not flushing
   ps_error[0] = '\0';       // no printer error

   state = NORMAL;           // reset msg parser
   msg.ptr = msg.buf;        // reset msg buffer
}

/*
 * Send a simple PostScript program that prompts
 * the printer to send back a PostScript message
 * containing the given cookie value:
 *
 *   %!PS
 *   (%%[ cookie: 12345 ]%%) print flush
 *
 * Return 0 if ok and -1/errno on failure.
 */
int psecho(int fd, int cookie)
{
   char *ps1 = "%!PS\n(%%[ cookie: ";
   char *ps2 = " ]%%) print flush\n";
   char buf[80]; // at least #ps1 + 10 + #ps2 + 2

   int n = snprintf(buf, sizeof buf, "%s%d%s\n", ps1, cookie, ps2);

   return pswrite(fd, buf, n);
}

/*
 * Send a simple PostScript program that reads the printer's
 * pagecount register and prints its value back to us:
 *
 *   %!PS
 *   (%%[ pagecount: ) print statusdict begin pagecount end
 *   20 string cvs print (; cookie: COOKIE ]%%) print flush
 *
 * Return 0 if ok and -1/errno on failure.
 */
int pscount(int fd, int cookie)
{
   char *ps1 = "%!PS\n(%%[ pagecount: ) print statusdict "
      "begin pagecount end 20 string cvs print (; cookie: ";
   char *ps2 = " ]%%) print flush";
   char buf[128]; // at least #ps1 + 10 + #ps2 + 2

   int n = snprintf(buf, sizeof buf, "%s%d%s\n", ps1, cookie, ps2);

   return pswrite(fd, buf, n);
}

/*
 * Write the given buffer to the given file descriptor
 * in blocking mode, that is, do not return until all
 * has been written or an error occurred.
 */
static int pswrite(int fd, const char *buf, unsigned len)
{
   return writeall(fd, buf, len);
}

/*
 * Parser for PostScript messages, which are expected to
 * be NUL-terminated strings. Invoke this parser whenever
 * a complete message was received from the printer.
 * Recognised messages:
 *
 *   %%[ cookie: *** ]%%
 *   %%[ pagecount: <number>; cookie: <number> ]%%
 *   %%[ PrinterError: <reason> ]%%
 *   %%[ Flushing: rest of job (to end-of-file) will be ignored ]%%
 *   %%[ Error: <error>; OffendingCommand: <command> ]%%
 *
 * The parser returns one of the PS_MSG_XXX constants to indicate
 * the type of message recognised.
 *
 * Note that PostScript printers on a serial line should respond
 * with a %%[ status: <status> ]%% message if they receive a ^T
 * (\024) command. This parser does not give special attention
 * to such messages.
 */
int psparse(const char *msg)
{
   const char *p;
   char *key, *val;
   int klen, vlen;

   //fprintf(stderr, "{psparse %s}\n", msg); //DEBUG

   p = msg;  key = val = 0;
   /* key remains unchanged if message malformed */
   p += getnext(p, &key, &klen, ':');
   p += getnext(p, &val, &vlen, ';');
   if (!key || !val) return PS_MSG_MALFORMED;

   /*
    * Handle %%[ cookie: <number> ]%% messages
    */
   if (strncmp(key, "cookie", klen) == 0) {
      if (scani(val, &ps_cookie) && (*p == 0))
         return PS_MSG_COOKIE;
      return PS_MSG_OTHER;
   }

   /*
    * Handle %%[ pagecount: <number>; cookie: <number> ]%% messages
    */
   if (strncmp(key, "pagecount", klen) == 0) {
      int n = scani(val, &ps_pagecount);
      if (n == 0) return PS_MSG_OTHER;

      if (*p) {
         if (psparse(p) != PS_MSG_COOKIE)
            return PS_MSG_OTHER;
      }
      else ps_cookie = 0; // no cookie
      return PS_MSG_PAGECOUNT;
   }

   /*
    * Handle %%[ PrinterError: <message> ]%% messages
    */
   if (strncmp(key, "PrinterError", klen) == 0) {
      strncpy(ps_error, val, sizeof ps_error);
      return PS_MSG_PRERROR;
   }

   /*
    * Handle %%[ Flushing: rest of job... ]%% messages
    */
   if (strncmp(key, "Flushing", klen) == 0) {
      ps_flushing = 1; // printer ignores further I/O
      return PS_MSG_FLUSHING;
   }

   return PS_MSG_OTHER;
}

/*
 * Get the next token and its length from 's', which is assumed
 * to point into a PostScript message.  A token starts at the
 * first non-space character after 's' and lasts until the first
 * 'stop' char, or null, whichever comes first.  Trailing white
 * space is not included in the length returned.  Return the
 * number of chars scanned, including a delimiting stop char.
 * Stop chars are ':' for the key and ';' for the value.
 */
static
int getnext(const char *s, char **tok, int *len, char stop)
{
   register const char *t, *w;
   register const char *p = s;

   if (!p || !*p) return 0;                 /* nothing to scan */

   while (isspace(*p)) p++;                 /* skip white space */
   t = p;                                   /* start scanning token */
   while (*p && (*p != stop)) p++;          /* until stop char or \0 */
   w = p;                                   /* backscan white space */
   while ((w > t) && isspace(*(w-1))) --w;

   if (tok) *tok = (char *) t;              /* token */
   if (len) *len = w - t;                   /* len of token */

   if (*p == stop) p++;                     /* skip stop char */
   else assert(*p == '\0');

   return p - s;                            /* #chars scanned */
}

/*
 * This routine is a simple state machine looking for
 * PostScript messages. If one is found, it is submitted
 * to psparse() and its value returned, otherwise, zero
 * will be returned as an indication that no message
 * was found so far. Call repeatedly for each character
 * read back from the printer.
 *
 * Note: This is NOT a general parser for PostScript
 * messages! It only works for those messages generated
 * by the routines above.
 */
int pschar(char c)
{
   int found = 0;

   switch (state) { /* FSM */
   case NORMAL:
      if (c == '%') state = GOTPERC;
      /* else keep state NORMAL */
      break;
   case INMSG:
      if (c == ']') state = GOTBRACK;
      else msgchar(c); // keep state
      break;
   case GOTPERC:
      if (c == '%') state = GOTPERCPERC;
      else state = NORMAL;
      break;
   case GOTPERCPERC:
      if (c == '[') state = INMSG;
      else if (c != '%') state = NORMAL;
      break;
   case GOTBRACK:
      if (c == '%') state = GOTBRACKPERC;
      else {
         msgchar(']');
         if (c == ']') state = GOTBRACK;
         else state = INMSG;
      }
      break;
   case GOTBRACKPERC:
      if (c == '%') { found = 1; state = NORMAL; }
      else { msgchar(']'); msgchar('%'); state = INMSG; }
      break;
   }
   return (found) ? psparse(msgterm()) : 0;
}

#ifdef TESTING
#include <stdio.h>
#include <string.h>
int main(void)
{
   char buf[1024];

   //pscount(1, 2147483647);
   printf("Type text including %%%%[ PostScript: messages ]%%%%\n");
   printf("and check if ps.c recognises and parses them!\n");
   fflush(stdout);

   psinit();
   while (fgets(buf, sizeof buf, stdin)) {
      register char *p = buf;
      while (*p) switch (pschar(*p++)) {
      case PS_MSG_COOKIE:
         printf("{ps_cookie=%ld}", ps_cookie);
         break;
      case PS_MSG_PAGECOUNT:
         printf("{ps_pagecount=%ld %ld}", ps_pagecount, ps_cookie);
         break;
      case PS_MSG_PRERROR:
         printf("{ps_error=%s}", ps_error);
         break;
      case PS_MSG_FLUSHING:
         printf("{ps_flushing=%d}", ps_flushing);
         break;
      case PS_MSG_OTHER:
         printf("{other}");
         break;
      case PS_MSG_MALFORMED:
         printf("{malformed}");
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
#endif /* TESTING */
