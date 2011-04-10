/* parser.c - part of pracc for CUPS sources
 * Copyright (c) 2005-2007 by Urs-Jakob Rueetschi.
 *
 * These routines parse printer output (which is input
 * for us) looking for PJL and PostScript messages.
 *
 *   PJL messages have the form:  @PJL ... <FF>
 *   PostScript messages have the form:  %%[ ... ]%%
 *
 * Ordinary input is sent to cupsBackChannelWrite(), while
 * PJL messages are handled by the PJL parser and PostScript
 * messages are handled by the PostScript parser.
 *
 * Note that the size of the message buffer must be large
 * enough to hold the largest message that we may generate!
 */

#ifdef TESTING
#include <stdio.h>
#define USEROUT(buf,len) printf("%*s\n", (len), (buf)) // DEBUG
#define pjlparse(msg,len) printf("{PJL:%*s}\n", (len), (msg))
#define psparse(msg,len) printf("{PS:%*s}\n", (len), (msg))
#else
#include <cups/backend.h>
#define USEROUT(buf,len) cupsBackChannelWrite((buf), (len), 1.0)
#endif

/* Message buffer */
static struct {
   char *ptr, *end;
   char buf[1024];
} msg = { msg.buf, msg.buf+1024 };

/* Initialise message buffer */
static inline void msginit(void)
{
   msg.ptr = msg.buf;
}

/* Append character to message buffer */
static void msgchar(char c)
{
   if (msg.ptr < msg.end) *msg.ptr++ = c;
   else /* buffer full, char silently lost */ ;
}

/* Append final NUL to message buffer */
static void msgterm(void)
{
   if (msg.ptr < msg.end) *msg.ptr++ = '\0';
   else *(msg.end-1) = '\0'; // overwrite
}

/* Length of message */
static inline int msglen(void)
{
   return msg.ptr - msg.buf;
}

/* User info buffer */
static struct {
   char *ptr, *end;
   char buf[1024];
} user = { user.buf, user.buf+1024 };

/*
 * Append a char to the user buffer.
 * Flush buffer to CUPS back channel if full.
 * A value of -1 forces a buffer flush.
 */ 
static
void userchar(int c)
{
   if (c >= 0) *user.ptr++ = c; // ASSUME sizeof buf > 0
   if (c < 0 || user.ptr == user.end) {
      USEROUT(user.buf, user.ptr-user.buf);
      user.ptr = user.buf; // reset buffer
   }
}

static enum { NORMAL, GOTPJL, PSMSG } state = NORMAL;
static int normalparse(char c);
static int psendparse(char c);

void parserinit(void)
{
   state = NORMAL;
   normalparse(0);
   psendparse(0);
}

void parseinput(const char *buf, unsigned len)
{
   register const char *p = buf;
   const char *end = buf + len;

   fprintf(stderr, "{parseinput:%*s}\n", len, buf);

   while (p < end) {
      char c = *p++;
      switch (state) { /* FSM */

      case NORMAL:
         state = normalparse(c);
         if (state != NORMAL) msginit();
         break;

      case GOTPJL:
         if (c == '\f') {
            state = NORMAL;
            msgterm(); // append NUL
            pjlparse(msg.buf, msglen());
            userchar(-1); // flush
         }
         else msgchar(c);
         break;
         
      case PSMSG:
         state = psendparse(c);
         if (state == NORMAL) {
            msgterm(); // append NUL
            psparse(msg.buf, msglen());
            userchar(-1); // flush
         }
         break;
      }
   }
}

/*
 * Look for "@PJL" or "%%[" from printer.
 * Return GOTPJL or PSMSG or NORMAL.
 */
static
int normalparse(char c)
{
   static enum {
      DEFLT,
      GOTAT, GOTATP, GOTATPJ,
      GOTPERC, GOTPERCPERC
   } state = DEFLT;

   switch (state) { /* FSM */
   case DEFLT: switch (c) {
         case '@': state = GOTAT; break;
         case '%': state = GOTPERC; break;
         default:  userchar(c); break;
      }
      break;
   case GOTAT: switch (c) { /* ...@ */
         case 'P': state = GOTATP; break;
         case '@': state = GOTAT; userchar('@'); break;
         case '%': state = GOTPERC; userchar('@'); break;
         default:  state = DEFLT; userchar('@'); userchar(c); break;
      }
      break;
   case GOTATP: switch (c) { /* ...@P */
         case 'J': state = GOTATPJ; break;
         case '@': state = GOTAT; userchar('@'); userchar('P'); break;
         case '%': state = GOTPERC; userchar('@'); userchar('P'); break;
         default:  state = DEFLT;
            userchar('@'); userchar('P'); userchar(c); break;
      }
      break;
   case GOTATPJ: switch (c) { /* ...@PJ */
         case 'L': state = DEFLT; return GOTPJL;
         case '@': state = GOTAT; userchar('@');
            userchar('P'); userchar('J'); break;
         case '%': state = GOTPERC; userchar('@');
            userchar('P'); userchar('J'); break;
         default:  state = DEFLT; userchar('@');
            userchar('P'); userchar('J'); userchar(c); break;
      }
      break;
   case GOTPERC: switch (c) { /* ...% */
         case '%': state = GOTPERCPERC; break;
         case '@': state = GOTAT; userchar('%'); break;
         default:  state = DEFLT; userchar('%'); userchar(c); break;
      }
      break;
   case GOTPERCPERC: switch (c) { /* ...%% */
         case '[': state = DEFLT; return PSMSG;
         case '@': state = GOTAT; userchar('%'); userchar('%'); break;
         case '%': /* keep state */ userchar('%'); break;
         default:  state = DEFLT; userchar('%');
            userchar('%'); userchar(c); break;
      }
      break;
   }
   return NORMAL;
}

/*
 * Look for the "]%%" that terminates a PS message
 */
static
int psendparse(char c)
{
   static enum {
      DEFLT,
      GOTBRACK, GOTBRACKPERC
   } state = DEFLT;

   switch (state) { /* FSM */
   case DEFLT:
      if (c == ']') state = GOTBRACK;
      else msgchar(c); /* keep state */
      break;
   case GOTBRACK:
      if (c == '%') state = GOTBRACKPERC;
      else if (c == ']') msgchar(']');
      else { msgchar(']'); msgchar(c); state = DEFLT; }
      break;
   case GOTBRACKPERC:
      if (c == '%') { state = DEFLT; return NORMAL; }
      else { msgchar(']'); msgchar('%'); msgchar(c); state = DEFLT; }
      break;
   }
   return PSMSG;
}

#ifdef TESTING
#include <stdio.h>
#include <string.h>
int main(void)
{
   char buf[1024];

   parserinit();
   while (fgets(buf, sizeof buf, stdin)) {
      int len = strlen(buf)-1;
      if (len >= 0) buf[len] = '\0';
      parseinput(buf, len);
      userchar(-1); // flush
   }

   return 123;
}
#endif /* TESTING */
