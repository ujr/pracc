/* 
 * Scan print job data and use a heuristic to tell
 * how many pages the job would produce when printed.
 *
 * This version only looks for a "magic" first line
 * that explicitly states the jobpages, or handles
 * DSC conformant PostScript by looking for %%Pages
 * comments.
 */

#define JOBDEBUG // copy job to /tmp/job.data (overwrite)

#include <ctype.h>
#include <string.h>
#ifdef JOBDEBUG
#include <stdio.h>
#endif

#ifdef TESTING
#include <stdio.h>
#define log_debug printf
#endif

#include "scan.h"

long jobpages = -1; // initially unknown
int jobscan(const char *buf, unsigned len);

static int atend = 0;
static int gotpages = 0;
static int jobcopies = 1; // default
static int gotcopies = 0;

/*
 * Look for the "magic" string ##Npp\n where N is an unsigned
 * decimal number that counts the number of pages in the job.
 * Return the number of "magic" bytes or zero if no match.
 */
static int checkmagic(const char *line)
{
   const char *p = line;
   unsigned long num;
   int n;

   if ((n = scans(p, "##"))) { p += n;
      if ((n = scanu(p, &num))) { p += n;
         if ((n = scans(p, "pp"))) { p += n;
            if (*p) return 0;
            jobpages = (signed long) num;
            gotpages = 1;
            jobcopies = 1;
            gotcopies = 1;
log_debug("Got magic ##%ldpp", num);
            return p - line; // #bytes to skip
         }
      }
   }
   return 0; // not our magic...
}

static void scan_ps_line(const char *line, unsigned len)
{
   const char *s;
   const char *p = line;
   unsigned long num;
   int n;

   if (!gotcopies && (n = scans(p, "%%Requirements:"))) {
      p += n;
      s = strstr(p, "numcopies");
      if (s) p = s + 9; else return;
      if ((s = strstr(p, "numcopies"))) p = s+9;
      while ((*p == ' ') || (*p == '\t')) ++p;
      if ((*p++ == '(') && (n = scanu(p, &num)) && (*(p+n) == ')')) {
         jobcopies = num;
         jobpages *= num;
         gotcopies = 1;
      }
   }
   if (!gotpages && (n = scans(p, "%%Pages:"))) {
      p += n;
      while ((*p == ' ') || (*p == '\t')) ++p;
      if ((n = scans(p, "(atend)"))) atend = 1;
      else if ((n = scanu(p, &num))) {
         jobpages = num * jobcopies;
         if (!atend) gotpages = 1;
      }
   }
   if (n = scans(p, "%%EndComments")) {
      jobcopies = 1; // assumption
      gotcopies = 1;
   }
}

/*
 * Scan the given buffer with job data, looking for
 * an indication of how many pages this job will print.
 * Return the number of bytes to skip over "magic" stuff
 * at the beginning of the print job or 0 if none.
 */
int jobscan(const char *buf, unsigned len)
{
   static char line[256];
   char *lp = line; // pointer into target line
   const char *linend = line + sizeof(line) - 1;
   const char *sp = buf; // pointer into source buf
   const char *bufend = buf + len; // sp < bufend
   char c;
   int n;

   if (gotpages && gotcopies) return 0; // no more scanning
   if (!buf) { atend = 0; gotpages = gotcopies = 0; return 0; } // reset

#ifdef JOBDEBUG
   {static FILE *fp = 0;
   if (!fp) fp = fopen("/tmp/job.data", "w");
   fwrite(buf, len, 1, fp); fflush(fp);}
#endif // JOBDEBUG

   while (sp < bufend) {
      switch (c = *sp++) {
         case '\n':
         case '\r':
         case '\0':
            *lp++ = '\0'; // terminate!
            if ((n = checkmagic(line)))
               return n+1; // incl \n
            scan_ps_line(line, lp-line);
            lp = line; // reset
            break;
         default:
            if (lp < linend) *lp++ = c;
            else /* drop silently */ ;
            break;
      }
   }
   return 0;
}

#ifdef TESTING
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
int main(int argc, char **argv)
{
   char buf[1024];
   int fd, n;

   if (argv && *argv) ++argv;
   else return 127; // no arg0?

   if (*argv) fd = open(*argv++, O_RDONLY);
   else fd = 0; // stdin by default
   if (fd < 0) return 111;

   while ((n = read(fd, buf, sizeof(buf))) > 0)
      jobscan(buf, n);
   if (n < 0) return 112;

   printf("Pages: %d\n", jobpages);

   return 0; // SUCCESS
}
#endif // TESTING
