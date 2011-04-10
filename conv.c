#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "getln.h"
#include "pracc.h"
#include "tai.h"

#define min(x,y) (((x) < (y)) ? (x) : (y))

long lineno = 0;

static void warn(const char *s);
static void die(const char *s);

int main(int argc, char **argv)
{
   char line[MAXLINE], *p, *q;
   char username[MAXNAME];
   char comment[MAXLINE];
   struct tai tai;
   int n, i, type;
   long num, value;
   time_t tstamp;
   const char *owner;

   owner = (argc > 1) ? argv[1] : NULL;

   while ((n = getln(stdin, line, sizeof(line), 0)) > 0)
   {
      p = line;
      lineno += 1;

      if (line[n-1] == '\n') line[n-1] = '\0';

      switch (type = *p++) {
         case '-': // debit
         case '+': // credit
            if (*p == '-') {
               warn("negative debit/credit value; skipping line");
               continue;
            }
            // FALLTHRU
         case '=': // reset
            if (n = scani(p, &num)) p += n;
            else {
               warn("expected number for debit/credit/reset; skipping line");
               continue;
            }
            value = num;
            break;
         case '$': // limit
            if (n = scani(p, &num)) p += n;
            else if (*p++ == '*') num = UNLIMITED;
            else {
               warn("expected number or '*' for limit; skipping line");
               continue;
            }
            value = num;
            break;
         case '!': // error
            break;
         case '#': // note
            goto comment;
         case '?': // obsolete
            warn("skipping obsolete ?-line");
            continue;
         default:
            warn("unknown line type; skipping line");
            continue;
      }

      p += scanpat(p, " ");

      if (n = taiscan(p, &tai)) p += n;
      else if (n = oldscan(p, &tai)) {
         p += n;
         warn("converting old timestamp format");
      }
      else die("invalid timestamp format; giving up");
      tstamp = taiunix(&tai);// - 3600;

      q = p;
      p += scanpat(p, " ");
      for (i = 0; *p && (i < MAXNAME); i++) {
         if (isspace(*p)) break;
         else username[i] = *p++;
      }
      username[min(i,MAXNAME-1)] = '\0';

      // User field differs from owner?
      if (owner && strcmp(username, owner)) {
         // User field differs from root and pracc?
         if (strcmp(username, "root") && strcmp(username, "pracc")) {
            p = q;
            strncpy(username, owner, MAXNAME-1);
            username[MAXNAME-1] = '\0';
            warn("adding user field");
         }
      }

comment:
      p += scanpat(p, " ");
      for (i = 0; *p && (i < MAXLINE); i++)
         comment[i] = *p++;
      comment[min(i, MAXLINE-1)] = '\0';

      /* Successfully scanned one line of input */

      praccAssemble(line, type, value, tstamp, username, comment);
      fputs(line, stdout);
   }
   if (n < 0) die("error reading input file; giving up");
   fprintf(stderr, "Successfully converted %ld lines\n", lineno);

   return 0;
}

static void warn(const char *s)
{
   fprintf(stderr, "line %d: %s\n", lineno, s);
}

static void die(const char *s)
{
   fprintf(stderr, "line %d: %s\n", lineno, s);
   exit(127);
}
