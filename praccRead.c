#include "pracc.h"

#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

#include "getln.h"
#include "tai.h"

#define min(x,y) (((x) < (y)) ? (x) : (y))

/*
 * Open a connection to the pracc file for the given account.
 * Return 0 if successful and -1 on error (errno has details).
 */
int praccOpen(const char *acctname, struct praccbuf *pracc)
{
   const char *fn;

   assert(pracc);

   fn = praccPath(acctname);
   if (!fn) return -1;

   pracc->fn = fn;
   pracc->lineno = 0;
   pracc->fp = fopen(fn, "r");
   if (!pracc->fp) {
      int saverr = errno;
      praccClose(pracc);
      errno = saverr;
      return -1;
   }

   return 0; // SUCCESS
}

/*
 * Read one record from the given pracc file connection.
 * Silently skip malformed entries and comment lines.
 *
 * Return 1 if the entry was successfully read, 0 if an
 * end-of-file was encountered, and -1 on error (in this
 * case errno has details).
 */
int praccRead(struct praccbuf *pracc)
{
   char line[MAXLINE], *p;
   struct tai tai;
   long num;
   int n, i;

   assert(pracc && pracc->fp);
next: p = line;
   n = getln(pracc->fp, line, sizeof(line), 0);
   if (n > 0) {
      pracc->lineno += 1;

      /* Kill the newline */
      if (line[n-1] == '\n') line[n-1] = '\0';

      pracc->type = (int) *p++;
      pracc->value = 0;
      pracc->tstamp = 0;
      pracc->username[0] = '\0';
      pracc->comment[0] = '\0';

      switch (pracc->type) {
         case '-': // debit
         case '+': // credit
            if (*p == '-') goto next; // skip bad line
            // FALLTHRU
         case '=': // reset
            if (n = scani(p, &num)) p += n;
            else goto next; // skip bad line
            pracc->value = num;
            break;
         case '$': // limit
            if (n = scani(p, &num)) p += n;
            else if (*p++ == '*') num = UNLIMITED;
            else goto next; // skip bad line
            pracc->value = num;
            break;
         case '!': // error
            break;
         case '#': // note
            goto comment;
         default: // skip
            goto next;
      }

      p += scanpat(p, " ");
      if (n = taiscan(p, &tai)) p += n;
      else
//#if OLDSTAMP
//         if (n = oldscan(p, &tai)) p += n;
//         else
//#endif
         return 1; // ok but incomplete
      pracc->tstamp = taiunix(&tai);

      p += scanpat(p, " ");
      for (i = 0; *p && (i < MAXNAME); i++) {
         if (isspace(*p)) break;
         else pracc->username[i] = *p++;
      }
      pracc->username[min(i,MAXNAME-1)] = '\0';

comment:
      p += scanpat(p, " ");
      for (i = 0; *p && (i < MAXLINE); i++)
         pracc->comment[i] = *p++;
      pracc->comment[min(i,MAXLINE-1)] = '\0';

      return 1; // SUCCESS
   }

   return (n < 0) ? -1 : 0; // error or eof
}

/*
 * Close a pracc file connection that was previously
 * created with a call to praccOpen().
 *
 * Return 0 if successful and -1 on error.
 */
int praccClose(struct praccbuf *pracc)
{
   assert(pracc);

   if (pracc->fp) {
      fclose(pracc->fp);
      pracc->fp = 0;
   }
   if (pracc->fn) {
      free((void *) pracc->fn);
      pracc->fn = 0;
   }
   pracc->lineno = 0;
   return 0; // SUCCESS
}

