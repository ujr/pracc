#include <assert.h>
#include <stdlib.h>
#include <time.h>

#include "pracc.h"
#include "print.h"
#include "tai.h"

/*
 * Assemble the individual fields of a pracc record
 * as a pracc line into the given buffer. The buffer
 * must provide space for at least MAXLINE characters.
 * The taip and username arguments must not be NULL.
 *
 * Return the number of characters written to the buffer
 */
static int praccAssembleTAI(buf, type, value, taip, username, comment)
   char *buf;
   int type;
   long value;
   struct tai *taip;
   const char *username;
   const char *comment;
{
   char *p, *endp;

   assert(buf != NULL);
   assert(taip != NULL);
   assert(username != NULL);

   p = buf;
   endp = buf + MAXLINE - 2;

   *p++ = (char) (unsigned char) type;
   switch (type) {
      case '-': // debit
      case '+': // credit
         assert(value >= 0); // BUG
         p += printu(p, value);
         break;
      case '=': // reset
         p += printi(p, value);
         break;
      case '$': // limit
         if (value == UNLIMITED) *p++ = '*';
         else p += printi(p, value);
         break;
      case '!': // error
         break;
      case '#': // note
         goto justinfo;
      default:
         abort(); // BUG
   }

   p += printc(p, ' ');
   p += taifmt(p, taip);

   p += printc(p, ' ');
   p += praccFormatName(username, p, MAXNAME);

justinfo: if (comment) {
      p += printc(p, ' ');
      p += praccFormatInfo(comment, p, endp-p);
   }

   p += printc(p, '\n');
   *p = '\0'; // terminate

   return p - buf; // #chars written to buf, incl \n, excl \0
}

int praccAssemble(buf, type, value, tstamp, username, comment)
   char *buf;
   int type;
   long value;
   time_t tstamp;
   const char *username;
   const char *comment;
{
   struct tai tai;

   assert(buf != NULL);
   assert(username != NULL);

   unixtai(&tai, tstamp);

   return praccAssembleTAI(buf, type, value, &tai, username, comment);
}

