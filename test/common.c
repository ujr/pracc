/* common.c - part of the pracc sources
 * $Id$
 * Copyright (c) 2005-2008 by Urs Jakob Ruetschi
 */

#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "common.h"
#include "pracc.h"

#if 0
/*
 * Get the program name from the given arguments array,
 * which is the basename of the first argument, argv[0].
 *
 * Return pointer to progname or NULL if no such argument.
 */
char *progname(char **argv)
{
   const char *s = 0;
   register const char *p;

   if (argv && *argv) {
      p = s = *argv;
      while (*p) if (*p++ == '/')
         if (p) s = p; // advance
   }
   return (char *) s;
}
#endif

#if 0
/*
 * Translate a pracc record type to its name.
 */
char *strtype(char type)
{
   switch (type) {
      case '-': return "debit";
      case '+': return "credit";
      case '=': return "reset";
      case '$': return "limit";
      case '!': return "error";
      case '?': return "error"; // legacy
      case '#': return "note";
      default:  return "other";
   }
}
#endif

#define NUL '\0'
#define SEP '\n'

int skipline(FILE *fp)
{
   register char c;

   if (fp) do { c = getc(fp); }
   while ((c != SEP) && (c != EOF));

   return c; // SEP or EOF
}

int getln(FILE *fp, char *buf, int size)
{
   if (fgets(buf, size, fp)) return strlen(buf);
   return (ferror(fp)) ? -1 : 0; // error or eof
}

int putln(FILE *fp, const char *s)
{
   register char c;
   register const char *p = s;
   while (*p) putc(c = *p++, fp);
   if (c != SEP) putc(SEP, fp);
   return (ferror(fp)) ? -1 : 0;
}

int putfmt(FILE *fp, const char *fmt, ...)
{
   int status;
   va_list ap;

   va_start(ap, fmt);
   status = vfprintf(fp, fmt, ap);
   va_end(ap);

   return (status < 0) ? -1 : 0;
}

int putbuf(FILE *fp, const char *buf, int size)
{
   size_t n = fwrite(buf, size, 1, fp);
   return (ferror(fp)) ? -1 : 0;
}

#if 0
/*
 * Reading a file line-by-line:
 *
 *   int getinit(const char *fn);
 *   int getln(char *buf, int size, int *partial);
 *
 * Getinit() opens the named file for reading by getln().
 * Getln() reads a line from input, puts the first size-1
 * characters read into buf, and appends \0 to make buf a
 * valid C string. It returns the number of characters put
 * into buf (incl \n but excl \0), 0 on end-of-file (note
 * that an empty line consists of a \n), and -1 on error.
 * If the line is longer than size-1, partial is set to one,
 * otherwise it is set to zero.
 *
 *   if (getinit("filename") < 0)
 *      error("cannot open filename");
 *   while ((n = getln(line, sizeof line, &partial)) > 0) {
 *      ...do sth with line...
 *      if (partial) ...complain... // optional
 *   }
 *   if (n < 0) error("error reading from filename");
 */

static FILE *getfp = 0;

int getinit(const char *fn)
{
   if (getfp) fclose(getfp);
   if (!fn) return 0; // ok
   if (!strcmp(fn, "-")) getfp = stdin;
   else getfp = fopen(fn, "r");
   return (getfp) ? 0 : -1;
}

int getln(char *buf, int size, int *partial)
{
   char *p = buf;
   char *end = buf + size - 1;
   register int c;

   while (p < end) {
      c = getc(getfp);
      if (c == EOF) break; // eof or error
      *p++ = (char) (unsigned char) c;
      if (c == SEP) break; // end-of-line
   }
   if (size > 0) *p = NUL;

   /* Flag partially read (ie, long) lines and errors */
   if (partial) *partial = (c == SEP) ? 0 : 1;
   if (c == EOF) return (ferror(getfp)) ? -1 : 0;

   /* Skip remainder of long line */
   if (c != SEP) do c = getc(getfp);
   while ((c != SEP) && (c != EOF));

   return p - buf; // #chars in buf, incl SEP, excl NUL
}

/*
 * Writing lines, that is, strings ending with \n
 *
 *   int putinit(const char *fn, int aflag);
 *   int putln(const char *s); // append \n if not present
 *
 * Putinit() opens the named file for subsequent writing
 * (aflag=0) or appending (aflag=1) of lines by putln().
 *
 * Putln() writes the given string. If the string does not
 * end with \n, an end-of-line \n is also written.
 */

static FILE *putfp = 0;

int putinit(const char *fn, int aflag)
{
   if (putfp) fclose(putfp);
   if (!fn) return 0; // ok, just close
   if (!strcmp(fn, "-")) putfp = stdout;
   else putfp = fopen(fn, aflag ? "a" : "w");
   return (putfp) ? 0 : -1;
}

int putln(const char *s)
{
   register char c;
   register const char *p = s;
   while (*p) putc(c = *p++, putfp);
   if (c != SEP) putc(SEP, putfp);
   return ferror(putfp) ? -1 : p-s;
}
#endif

/*
 * Atomically append to the common log file.
 * Return 0 on success, -1 on failure.
 */
int logup(const char *buf, int len)
{
   int fd;

   fd = open(PRACCLOG, O_CREAT | O_WRONLY | O_APPEND, 0660);
   if (fd < 0) return -1; // FAILURE
   if (write(fd, buf, len) != len) return -1;
   if (close(fd) < 0) return -1;

   return 0; // SUCCESS
}

#if 0
static int writeall(int fd, const char *buf, int len)
{
   while (len > 0) { int w;
      do w = write(fd, buf, len);
      while ((w < 0) && (errno == EINTR));
      if (w < 0) return -1; // see errno
      buf += w;
      len -= w;
   }
   return 0; // SUCCESS
}

/*
 * Output a buffer to stdout.
 * Return 0 if ok, -1 on error.
 */
int bufout(const char *buf, int len)
{
   // fwrite(buf, sizeof char, len, stdout);
   // return ferror(stdout) ? -1 : 0;
   return writeall(1, buf, len);
}

/*
 * Formatted output to stdout.
 * Return 0 if ok, -1 on error.
 */
int fmtout(const char *fmt, ...)
{
   char buf[1024];
   int status;
   va_list ap;

   va_start(ap, fmt);
   status = bufout(buf, formatv(buf, sizeof buf, fmt, ap));
   va_end(ap);

   return status;
}

/*
 * Output a buffer to stderr.
 * Return 0 if ok, -1 on error.
 */
int buferr(const char *buf, int len)
{
   // fwrite(buf, sizeof char, len, stderr);
   // return ferror(stderr) ? -1 : 0;
   (void) writeall(2, buf, len);
}

/*
 * Formatted output to stderr.
 * Return 0 if ok, -1 on error.
 */
int fmterr(const char *fmt, ...)
{
   char buf[1024];
   int status;
   va_list ap;

   va_start(ap, fmt);
   status = buferr(buf, formatv(buf, sizeof buf, fmt, ap));
   va_end(ap);

   return status;
}
#endif

#if 0
/*
 * Terminate program execution with the given status
 * code, after printing an error message to stderr.
 */
void die(int code, const char *fmt, ...)
{
   extern char *me;
   char buf[1024], *p = buf;
   char *end = buf + sizeof(buf);

   va_list ap;
   va_start(ap, fmt);

   p += format(p, end-p, "%s: ", me);
   p += formatv(p, end-p, fmt, ap);
   if (errno) // system error
      p += format(p, end-p, ": %s", strerror(errno));

   va_end(ap);

   if (p < end) *p++ = '\n';
   else *(p-1) = '\n';
   putbuf(stderr, buf, p-buf);

   exit(code);
}
#endif
