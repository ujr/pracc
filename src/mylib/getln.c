/* getln.c - part of my C library
 *
 * Copyright (c) 2008, 2009 by Urs-Jakob Ruetschi.
 * Published under the GNU General Public License.
 */

#include <stdio.h>

#define SEP '\n'
#define NUL '\0'

/**
 * Read from fp at most size-1 characters or up to and
 * including the first occurrence of '\n', storing the
 * characters read into the given buffer. Append '\0'
 * to the buffer (whence at most size-1 chars are read).
 *
 * Return the number of characters read, excluding '\0'
 * but including '\n'. Return 0 if the end-of-file is
 * reached. Return -1 if an error occurs.
 *
 * Typical use pattern:
 *
 *    char line[256];
 *    int n, skipped;
 *
 *    while ((n = getln(fp, line, sizeof line, &skipped)) > 0) {
 *       ...do sth with line...
 *    }
 *    if (n < 0) perror("error reading from ...");
 */
int getln(FILE *fp, char *buf, int size, int *skipped)
{
   char *p = buf;
   char *end = buf + size - 1;
   register int c;

   while (p < end) {
      c = getc(fp);
      if (c == EOF) break; // eof or error
      *p++ = (char) (unsigned char) c;
      if (c == SEP) break; // end-of-line
   }
   if (size > 0) *p = NUL;

   // Flag partial (ie, long) line and errors:
   if (skipped) *skipped = (c == SEP) ? 0 : 1;
   if (c == EOF) return (ferror(fp)) ? -1 : 0;

   // Skip remainder of long line:
   if (c != SEP) do c = getc(fp);
   while ((c != SEP) && (c != EOF));

   return p - buf; // #chars in buf, incl SEP, excl NUL
}

/**
 * Read and discart input from fp up to (and including)
 * the first occurrence of SEP or EOF, whichever comes first.
 * Return the number of characters read or -1 on error.
 */
int eatln(FILE *fp)
{
   register char c;
   register int n = 0;

   if (fp) do { c = getc(fp); ++n; }
   while ((c != SEP) && (c != EOF));

   return (c == SEP) ? n : -1;
}
