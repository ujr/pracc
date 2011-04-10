/* Scan a PosctScript document for its %%Pages comment.
 * Output the first one found or, if it is (atend), the
 * last one found. Output -1 if no valid %%Pages comment.
 *
 * ujr/2008-01-03
 */

#include <stdio.h>
#include <string.h>

long pages = -1; // unknown

int scans(const char *s, const char *t)
{
   register const char *p = t;
   while (*s && (*s == *p)) { ++s; ++p; }
   if (*p) return 0; // mismatch
   return p - t; // #chars scanned
}

int scanu(const char *s, unsigned long *num)
{
   unsigned long c, val = 0;
   int i = 0;

   if (s == 0) return 0;
   while ((c = (unsigned long) (s[i] - '0')) < 10) {
      val = 10 * val + c;
      i++;
   }
   if (num) *num = val;
   return i; // #chars scanned
}

void scanline(const char *line, unsigned len)
{
   static int gotit = 0;
   static int atend = 0;
   const char *p = line;
   int n;
   unsigned long num;

   if ((n = scans(p, "%%Pages:"))) p += n;
   if (gotit || n == 0) return;

   while (isspace(*p)) ++p; // skip opt space
   if ((n = scans(p, "(atend)"))) atend = 1;
   else if ((n = scanu(p, &num))) {
      pages = num;
      if (!atend) gotit = 1;
   }
}

void scanjob(const char *buf, unsigned len)
{
   static char line[256];
   const char *sp = buf; // pointer into source buf
   register char *lp = line; // pointer into line
   const char *end = line + sizeof line; // lp < end
   char c;

   while (c = *sp++) {
      switch (c) {
         case '\n':
         case '\r':
            scanline(line, lp - line);
            lp = line; // reset
            break;
         default:
            if (lp < end) *lp++ = c;
            else /* drop silently */ ;
            break;
      }
   }
}

int main(void)
{
   char buf[1024];
   int n;

   while ((n = read(0, buf, sizeof buf)) > 0)
      scanjob(buf, n);

   printf("pages: %d\n", pages);

   return 0;
}
