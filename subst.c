/* Substitution, $Revision: 1.3 $ */
/* Syntax motivated by CUPS cgi_copy() */
/* Urs-Jakob Rueetschi, March 2008 */
/* ujr/20080902 include files now relative to current file */

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "scan.h"
#include "symtab.h"

extern void install(const char *name, const char *value);
extern char *lookup(const char *name, const char *deflt);

extern void array_init(const char *name);
extern int array_load(const char *name, int index);

static char escape(char c);
static int compare(int op, const char *s, const char *t);
static char *qualify(const char *fn, const char *curfn);
static long getfirst(), getcount();
static void copy(FILE *in, FILE *out);

/*
 * {name}       substitute value of name
 * {$name}      substitute value of name, formatted as a currency
 * {name?defined:undefined}
 * {name=value?true:false}  also: < > %
 * {name#html}
 * {#}          iteration number or zero
 * {:file}      input and process file
 * {;file}      input file verbatim
 * Note: file is relative to the currently processed file!
 *
 * Escapes:
 *  X preceded by a backslash is X without its side effects.
 *  For example, say \? for ? (useful in value), \: for :, \{
 *  and \} for { and } (useful in html), and \\ for a backslash.
 *  A newline preceded by a backslash is ignored.
 *  Escapes are inactive outside the outmost {...} pair.
 */
void subst(FILE *in, FILE *out, char term, int num, int level)
{
   char name[256];
   char value[256];
   char *s;
   const char *end;
   long first, limit;
   long prev, next;
   long inpos;
   int c, n;
   int r = 0;
   int op, cmp;
   int esc = 0;

   while ((c = getc(in)) != EOF) {
      if (esc) {
         if (out && (c != '\n'))
            putc(escape(c), out);
         esc = 0; continue;
      }
      if (term && (c == '\\')) {
         esc = 1; continue;
      }
      if (c == term) break; // done
      if (c == '{') { // substitution
         s = name;
         end = s + sizeof(name) - 1;
         while ((c = getc(in)) != EOF) {
            if (strchr("}?=<>% \t\n\0", c)) break;
            if ((c == '#') && (s > name)) break;
            if (s < end) *s++ = c;
         }
         *s = '\0';

         if (s == name) { // lonely opening brace
            if (out)
               fprintf(out, "{%c", c);
            continue;
         }

         if (name[0] == ':') { // include and process file
            if (c == '}') {
               char *curfn = lookup("FILENAME", ".");
               char *fn = qualify(name+1, curfn);
               FILE *fp = fopen(fn, "r");
               
               if (fp) {
                     char *savefn = lookup("FILENAME", 0);
                     install("FILENAME", fn);
                     subst(fp, out, 0, num, level+1);
                     install("FILENAME", savefn);
                     fclose(fp);
               }
               else if (out)
                  fprintf(out, "{:%s: %s}", name+1, strerror(errno));
               free(fn);
            }
            else if (out)
               fprintf(out, "{%s%c", name, c);
            continue;
         }

         if (name[0] == ';') { // include file verbatim
            if (c == '}') {
               char *curfn = lookup("FILENAME", ".");
               char *fn = qualify(name+1, curfn);
               FILE *fp = fopen(fn, "r");
               if (fp) {
                  if (out) copy(fp, out);
                  fclose(fp);
               }
               else if (out)
                  fprintf(out, "{;%s: %s}", name+1, strerror(errno));
               free(fn);
            }
            else if (out)
               fprintf(out, "{%s%c", name, c);
            continue;
         }

         switch (c) {
            case '}':
               if (!out) break;
               if (!strcmp(name, "#")) // iteration number
                  fprintf(out, "%d", num);
               else if (name[0] == '$') { // currency
                  long number;
                  char *s = lookup(name+1, "");
                  int n = scani(s, &number);
                  if (n && (s[n] == '\0'))
                     fprintf(out, "%.02f", ((float) number)/100.0);
                  else fprintf(out, "%s", lookup(name+1, ""));
               }
               else fprintf(out, "%s", lookup(name, ""));
               break;
            case '?':
               cmp = (lookup(name, 0) != 0);
               if (cmp) { // name exists
                  subst(in, out, ':', num, level+1);
                  subst(in, NULL, '}', num, level+1);
               }
               else { // name does not exist
                  subst(in, NULL, ':', num, level+1);
                  subst(in, out, '}', num, level+1);
               }
               break;
            case '#':
               array_init(name);
               first = getfirst();
               limit = first + getcount();
               inpos = ftell(in); // remember current position
               if (inpos < 0) r = -1;
               else for (n = first; n < limit; n++) {
                  if ((r = array_load(name, n))) break;
                  if ((r = fseek(in, inpos, SEEK_SET))) break;
                  subst(in, out, '}', n, level+1);
               }
               if (n == first) subst(in, NULL, '}', n, level+1);
               if (r < 0) install("error", strerror(errno));
               else install("error", 0); // remove error

               /* Install "prev" link */
               if ((r == 0) && (first > 1)) {
                  char buf[16];
                  long prev = first - n + first;
                  if (prev < 1) prev = 1;
                  snprintf(buf, sizeof(buf), "%ld", prev);
                  install("prev", buf); // TODO make complete URL
               }
               else install("prev", 0);

               /* Install "next" link */
               if (r == 0) {
                  char buf[16];
                  snprintf(buf, sizeof(buf), "%ld", (long) n);
                  install("next", buf); // TODO make complete URL
               }
               else install("next", 0);

               break;
            case '=':
            case '<':
            case '>':
            case '%':
               op = c;
               s = value;
               end = s + sizeof(value) - 1;
               while ((c = getc(in)) != EOF) {
                  if (esc) {
                     esc = 0;
                     if ((s < end) && (c != '\n'))
                        *s++ = escape(c);
                  }
                  else {
                     if (c == '\\') esc = 1;
                     else {
                        if (strchr("?}\0", c)) break;
                        if (s < end) *s++ = c;
                     }
                  }
               }
               *s = '\0';
               esc = 0;

               if (c == '?') { // comparison
                  char buf[16], *s;
                  if (strcmp(name, "#") == 0) {
                     snprintf(buf, sizeof(buf), "%d", num);
                     s = buf;
                  }
                  else s = lookup(name, 0);
                  cmp = compare(op, s, value);
                  if (cmp) {
                     subst(in, out, ':', num, level+1);
                     subst(in, NULL, '}', num, level+1);
                  }
                  else {
                     subst(in, NULL, ':', num, level+1);
                     subst(in, out, '}', num, level+1);
                  }
               }
               else if ((c == '}') && (op == '=') && out) { // assignment
                  const char *n = strdup(name);
                  if (n) install(n, value);
                  else install("error", strerror(errno));
               }
               else if (out) // syntax error
                  fprintf(out, "{%s%c%s%c", name, op, value, c);
               break;
            default:
               if (out) fprintf(out, "{%s%c", name, c);
               break;
         }
      }
      else if (out) putc(c, out);
   }
}

static char escape(char c)
{
   switch (c) {
      case '0': return '\0'; // ASCII 0
      case 'a': return '\a'; // ASCII 7, alert
      case 'b': return '\b'; // ASCII 8, backspace
      case 't': return '\t'; // ASCII 9, tab
      case 'n': return '\n'; // ASCII 10, newline
      case 'v': return '\v'; // ASCII 11, vertical tab
      case 'f': return '\f'; // ASCII 12, form feed
      case 'r': return '\r'; // ASCII 13, carriage return
   }
   return c;
}

/* Compare s and t; return 1 if s op t, 0 otherwise */
static int compare(int op, const char *s, const char *t)
{
   long ns, nt;
   char *ps, *pt;

   if (s == t) return 1;
   if (!s || !t) return 0;

   ns = strtol(s, &ps, 10);
   nt = strtol(t, &pt, 10);

   if (!*ps && !*pt) switch (op) {
      case '=': return ns == nt;
      case '<': return ns < nt;
      case '>': return ns > nt;
      case '%': return (ns % nt) == 0;
   }
   else switch (op) {
      case '=': return strcmp(s, t) == 0;
      case '<': return strcmp(s, t) < 0;
      case '>': return strcmp(s, t) > 0;
   }
   return 0;
}

/* Qualify given filename relative to current file,
 * return it in a newly allocated piece of memory */
static char *qualify(const char *fn, const char *curfn)
{
   const char *s;
   char *buf;
   int n, len;

   if (fn[0] == '/') return strdup(fn);

   s = strrchr(curfn, '/');
   n = s ? s - curfn : 0;

   if (n == 0) return strdup(fn);

   len = n + 1 + strlen(fn) + 1;
   if ((buf = malloc(len)) == NULL) {
      perror("subst/qualify");
      exit(111);
   }

   memcpy(buf, curfn, n); // prefix
   snprintf(buf+n, len-n, "/%s", fn);

   return buf;
}

static long getfirst()
{
   char *s = lookup("first", "");
   if (s[0]) {
      long first = atol(s);
      if (first < 1) return 1;
      return first;
   }
   return 1; // default
}

static long getcount()
{
   char *s = lookup("count", "");
   if (s[0]) {
      long count = atol(s);
      if (count < 0) return 0;
      return count;
   }
   return 99999; // default
}

/* Just copy input to output for {;file} */
static void copy(FILE *in, FILE *out)
{
   int c;

   while ((c = getc(in)) != EOF)
      if (putc(c, out) == EOF) break;
}

/*** Driver for the substitution engine ***
 *
 * Usage: subst [-V] {a=b} {files}
 *
 * Arguments of the form a=b assign b to a; all other
 * arguments are taken as files to be substituted.
 *
 * Option -V shows version and quits.
 ***/

#ifdef STANDALONE

#include <time.h>
#include <unistd.h>

static char id[] = "$Id: subst.c,v 1.3 2008/09/03 20:52:32 ujr Exp ujr $";

struct symtab syms;

static char *stredup(const char *s)
{
   char *t = strdup(s);
   if (t) return t;
   perror("strdup");
   exit(111);
}

static void dumpsym(struct symbol *s)
{ if (s && s->sval) printf("%s=%s$\n", s->name, s->sval); }

char *lookup(const char *name, const char *deflt)
{
   struct symbol *s;

   if (!name) return (char *) deflt;

   s = symget(&syms, name);
   if (s) return (char *) (s->sval ? s->sval : deflt);

   if (!strcmp(name, "symtab")) {
      symdump(&syms, stdout);
      return "";
   }
   if (!strcmp(name, "symdump")) {
      symeach(&syms, dumpsym);
      return "";
   }

   return (char *) deflt;
}

void install(const char *name, const char *value)
{
   struct symbol *sym;

   sym = symput(&syms, name);
   if (sym) {
      if (sym->sval != value) {
         if (sym->sval) free((void *) sym->sval);
         if (value) value = stredup(value);
         sym->sval = value;
      }
      else /* never free(x) and then strdup(x) */ ;
   }
   else {
      perror("install failed");
      exit(111);
   }
}

/*
 * Called by subst() once before substituting an array.
 * This should prepare subsequent calls to array_load.
 */
void array_init(const char *name)
{
}

/*
 * Called by subst() for each array element.
 * Return 0 if OK, 1 if no such element,
 * and -1 on error (set errno).
 */
int array_load(const char *name, int num)
{
   return 1;
}

void setup(void)
{
   char buf[256];
   time_t now = time(0);
   struct tm *tp = localtime(&now);
   if (tp) {
      snprintf(buf, sizeof(buf), "%d", 1900+tp->tm_year);
      install("YEAR", buf);
      snprintf(buf, sizeof(buf), "%d", tp->tm_mon + 1);
      install("MONTH", buf);
      snprintf(buf, sizeof(buf), "%d", tp->tm_mday);
      install("DAY", buf);
      snprintf(buf, sizeof(buf), "%d", 100*tp->tm_hour + tp->tm_min);
      install("TIME", buf);
   }
   if (gethostname(buf, sizeof(buf)) == 0) {
      buf[sizeof(buf)-1] = '\0';
      install("HOSTNAME", buf);
   }
}

int main(int argc, char **argv)
{
   FILE *infp;
   char *me, *s;
   char buf[256];
   int c;

   extern int optind;
   extern int opterr;
   extern char *progname(char **argv);

   me = progname(argv);
   if (!me) return 127;

   opterr = 0; // prevent stupid getopt output
   while ((c = getopt(argc, argv, "V")) > 0) switch (c) {
      case 'V': printf("%s\n", id); return 0;
      default: fprintf(stderr, "Usage: %s [-V] {name=value} {files}\n", me);
   }
   argc -= optind;
   argv += optind;

   syminit(&syms, 100);
   setup();

   while (*argv) {
      const char *arg = *argv++;

      if (s = strchr(arg, '=')) {
         *s++ = '\0'; // XXX
         install(arg, s);
      }
      else {
         install("FILENAME", arg);
         if (!(infp = fopen(arg, "r"))) {
            fprintf(stderr, "%s: open %s: %s\n",
               me, arg, strerror(errno));
            return 111;
         }
         if (fseek(infp, 0, SEEK_CUR)) {
            fprintf(stderr, "%s: %s: Not seekable\n", me, arg);
            return 127;
         }
         subst(infp, stdout, 0, 0, 0);
      }
   }
   return 0;
}
#endif
