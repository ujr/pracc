#include <ctype.h>
#include <errno.h>
#include <locale.h>
#include <pwd.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include "cgi.h"

/* borrowed from subst.c */
extern void install(const char *name, const char *value);
extern char *lookup(const char *name, const char *deflt);
extern void subst(FILE *in, FILE *out, char term, int num, int level);

/*
 * The CGI code here references these templates:
 *
 *   header.tmpl    in  cgiStartHTML()
 *   footer.tmpl    in  cgiEndHTML()
 *   error.tmpl     in  cgiError()
 *
 * Templates are assumed to be in PRACCDOC/tmpl,
 * where PRACCDOC is defined in the pracc.h header.
 */

static void parse_query_string(const char *qs);
static char *urldecode(char *s);

static const char *_cgiDataDir = (char *) 0;

#define streq(s,t) (strcmp((s),(t)) == 0)

/*
 * Initialise the CGI stuff by parsing form data, if any,
 * and installing commonly useful symbols, in particular:
 *
 *  SCRIPT, SCRIPT_NAME,   http://host/THIS/SCRIPT/path/info?query
 *  PATH,   PATH_INFO,     http://host/this/script/PATH/INFO?query
 *  QUERY,  QUERY_STRING;  http://host/this/script/path/info?QUERY
 *
 *  YEAR, MONTH, DAY, TIME; HOSTNAME;
 *  UID, EUID, GID, EGID; USER.
 *
 * Return 1 if there's form data, 0 otherwise.
 */
int cgiInit(const char *cgiDataDir)
{
   char buf[64], *ptr;
   time_t now = time(0);
   struct tm *tp;
   struct passwd *pw;
   char *method;
   char *lang;

   /* Add some CGI vars */

   install("SERVER_NAME", getenv("SERVER_NAME"));
   install("HTTP_USER_AGENT", getenv("HTTP_USER_AGENT"));
   install("HTTP_REFERER", getenv("HTTP_REFERER"));

   install("REMOTE_ADDR", getenv("REMOTE_ADDR"));
   install("REMOTE_HOST", getenv("REMOTE_HOST"));
   install("REMOTE_USER", getenv("REMOTE_USER"));

   install("REQUEST_METHOD", getenv("REQUEST_METHOD"));
   install("SCRIPT_NAME", getenv("SCRIPT_NAME"));
   install("PATH_INFO", getenv("PATH_INFO"));
   install("QUERY_STRING", getenv("QUERY_STRING"));

   /* Useful shortcuts */

   install("SCRIPT", getenv("SCRIPT_NAME"));
   install("PATH", getenv("PATH_INFO"));
   install("QUERY", getenv("QUERY_STRING"));

   /* Add user IDs */

   sprintf(buf, "%d", (int) getuid());
   install("UID", buf);

   sprintf(buf, "%d", (int) geteuid());
   install("EUID", buf);

   sprintf(buf, "%d", (int) getgid());
   install("GID", buf);

   sprintf(buf, "%d", (int) getegid());
   install("EGID", buf);

   pw = getpwuid(getuid());
   if (pw) install("USER", pw->pw_name);

   /* Date/time and hostname */

   if ((tp = localtime(&now))) {
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

   /* Store Accept-Language info, if any, in LANG */

   if ((lang = getenv("HTTP_ACCEPT_LANGUAGE"))) {
      lang = strdup(lang); // our own copy
      ptr = lang;
      while (isalpha(*ptr) || *ptr == '-') ++ptr;
      *ptr = '\0'; // cut invalid tail
      install("LANG", lang);
   }

   /* Set template base directory */

   if (cgiDataDir && cgiDataDir[0])
      _cgiDataDir = cgiDataDir;
   else _cgiDataDir = "/tmp"; // default

   /* Get form data, if any */

   method = getenv("REQUEST_METHOD");
   if (!method) return 0; // not CGI

   if (strcasecmp(method, "GET") == 0) {
      char *qs = getenv("QUERY_STRING");
      if (!qs) return 0; // no query
      parse_query_string(qs);
      return 1; // OK
   }

   if (strcasecmp(method, "POST") == 0) {
      int len, n, m;
      char *str;
      //char *ct = getenv("CONTENT_TYPE");
      char *cl = getenv("CONTENT_LENGTH");

      // Check multipart: TODO

      if ((len = atoi(cl)) <= 0) return 0; // no query
      if ((str = malloc(len+1)) == 0) return 0; // ENOMEM

      for (m = 0; m < len; m += n) {
         n = read(0, str+m, len-m);
         if ((n < 0) && (errno != EAGAIN)) {
            free(str);
            return 0; // I/O error
         }
      }
      str[len] = '\0';
      parse_query_string(str);
      return 1; // OK
   }
      
   return 0; // unknown method
}

/**
 * Add all arguments of the form name=value to the
 * repository of CGI variables; skip all other args.
 */
void cgiSetFromArgs(int argc, char **argv)
{
   char buf[1024], *p;

   if (argv) while (*argv && (argc > 0)) {
      if (strlen(*argv) < sizeof(buf)) {
         strcpy(buf, *argv);
         p = strchr(buf, '=');
         if (p && (p > buf)) {
            *p++ = '\0';
            install(buf, p);
         }
      }
      --argc;
      ++argv;
   }
}

void cgiStartHTML(const char *title)
{
   printf("Content-type: text/html\r\n\r\n");
   install("TITLE", title);
   cgiCopyTemplate("header.tmpl");
}

void cgiEndHTML(void)
{
   cgiCopyTemplate("footer.tmpl");
}

/*
 * Send the HTTP headers for downloading a CSV file.
 * See RFC 2183 for details.
 * Content-disposition is a disputed (but handy) feature.
 */
void cgiStartCSV(const char *filename)
{
   printf("Content-type: text/csv\r\n");

   if (filename) {
      printf("Content-disposition: attachment; filename=%s\r\n\r\n",
             filename);
   }
}

void cgiCopyTemplate(const char *tmplname)
{
   const char *lang = lookup("LANG", 0);
   cgiCopyTemplateLang(tmplname, lang);
}

void cgiCopyTemplateLang(const char *tmplname, const char *lang)
{
   FILE *fp;
   char fn[1024];
   char *tmpldir;
   char locale[16];
   char *p;

   tmpldir = cgiTemplateBase(); // never NULL

   if (lang) {
      snprintf(locale, sizeof(locale), "%s", lang);
      if ((p = strchr(locale, '.'))) *p = '\0';
      if ((p = strchr(locale, '/'))) *p = '\0';

      snprintf(fn, sizeof(fn), "%s/%s/%s", tmpldir, locale, tmplname);
      fp = fopen(fn, "r");

      if (!fp) {
         locale[2] = '\0'; // cut region

         snprintf(fn, sizeof(fn), "%s/%s/%s", tmpldir, locale, tmplname);
         fp = fopen(fn, "r");

         if (!fp) {
            snprintf(fn, sizeof(fn), "%s/%s", tmpldir, tmplname);
            fp = fopen(fn, "r");
         }
      }
   }
   else {
      snprintf(fn, sizeof(fn), "%s/%s", tmpldir, tmplname);
      fp = fopen(fn, "r");
   }

   if (fp) {
      install("FILENAME", fn);
      subst(fp, stdout, 0, 0, 0);
      fclose(fp);
   }
   else {
      printf("<p>Cannot open template file %s<br>\n", fn);
      printf(" ERROR: <b>%s</b></p>\n", strerror(errno));
   }
}

void cgiCopyVerbatim(const char *filename, const char *mimetype)
{
   FILE *fp;
   char buf[8192];
   size_t n;
   int ok;

   fp = fopen(filename, "r");
   if (fp) {
      if (mimetype)
         printf("Content-type: %s\r\n", mimetype);
      printf("\r\n"); // empty line terminates headers
      do {
         n = fread(buf, 1, sizeof(buf), fp);
         if (n > 0) fwrite(buf, 1, n, stdout);
      }
      while (n > 0);

      ok = feof(fp);
      fclose(fp);

      if (ok) return;
   }

   printf("Content-type: text/plain\r\n\r\n");
   printf("Cannot copy %s to standard output\n", filename);
   if (errno) printf("ERROR: %s\n", strerror(errno));
}

/*
 * Issue an error message by
 * 1. sending the error.tmpl template to the client
 * 2. writing the error message to stderr (browser log)
 */
void cgiError(const char *fmt, ...)
{
   char s[1024];
   va_list va;

   va_start(va, fmt);
   vsnprintf(s, sizeof(s), fmt, va);
   install("message", s);
   fprintf(stderr, "%s\n", s);
   va_end(va);

   if (errno) {
      install("error", strerror(errno));
      fprintf(stderr, "ERROR: %s\n", strerror(errno));
   }

   cgiCopyTemplate("error.tmpl");
}

char *cgiPathPrefix(const char *path, const char *prefix)
{
   if (path && prefix) {
      int n = strlen(prefix);
      if (strncasecmp(path, prefix, n) == 0)
         if (path[n] == '\0' || path[n] == '/')
            return (char *) (path+n);
   }
   return (char *) 0;
}

/*
 * Guess a document's MIME type from its file name extension.
 * Return a constant string with the MIME type or NULL if unknown.
 */
char *cgiGuessMimeType(const char *fn)
{
   char *p = strrchr(fn, '.');
   if (p++) {
      if (streq(p, "css") || streq(p, "CSS")) return "text/css";
      if (streq(p, "png") || streq(p, "PNG")) return "image/png";
      if (streq(p, "jpeg") || streq(p, "JPEG")) return "image/jpeg";
      if (streq(p, "jpg") || streq(p, "JPG")) return "image/jpeg";
      if (streq(p, "txt") || streq(p, "TXT")) return "text/plain";
      if (streq(p, "html") || streq(p, "HTML")) return "text/html";
      if (streq(p, "htm") || streq(p, "HTM")) return "text/html";
      if (streq(p, "gif") || streq(p, "GIF")) return "image/gif";
      if (streq(p, "pdf") || streq(p, "PDF")) return "application/pdf";
      if (streq(p, "svg") || streq(p, "SVG")) return "image/svg+xml";
      if (streq(p, "csv") || streq(p, "CSV")) return "text/csv";
   }
   return (char *) 0; // unknown
}

/*
 * Return base directory for templates, no trailing slash.
 */
char *cgiTemplateBase(void)
{
   static char tmpldir[1024] = "";

   if (!tmpldir[0])
      snprintf(tmpldir, sizeof(tmpldir), "%s/tmpl", _cgiDataDir);

   return tmpldir;
}

#if 0
/*
 * Return base directory for related documents such as
 * images, documentation, templates, etc.
 */
char *cgiDocumentBase(void)
{
   return PRACCDOC;
}
#endif

#if 0
/*
 * Split up the PATH_INFO CGI variable into up to 9
 * variables sect1..sect9 in the global symbol table.
 */
void split_path_info(const char *s)
{
   const char *p = s;
   const char *pp = 0;
   struct symbol *sym;
   char sectname[] = "sectN\0";
   int sectnum = 0;

   if (p) while (*p) {
      if (*p == '/') {
         if (pp) {
            if (++sectnum > 9) break;
            sectname[4] = (char) sectnum + '0';
            sym = symput(&syms, bdup(sectname, 5));
            if (sym) sym->sval = bdup(pp, p-pp);
         }
         pp = ++p;
      }
      else ++p;
   }
   if (pp && *pp && (++sectnum <= 9)) {
      sectname[4] = (char) sectnum + '0';
      sym = symput(&syms, bdup(sectname, 5));
      if (sym) sym->sval = bdup(pp, p-pp);
   }
}
#endif

/*
 * Parse the given query string and add all name=value
 * pairs to our symbol table; urldecode value but not name.
 */
static void parse_query_string(const char *qs)
{
   char name[256], *namep;
   const char *nameend;
   char value[1024], *valuep;
   const char *valueend;
   register const char *p = qs;

   nameend = name + sizeof(name) - 1;
   valueend = value + sizeof(value) - 1;

//fprintf(stderr, "parse_query_string(%s)\n", qs);//DEBUG

   if (p) while (*p) {
      namep = name;
      while (*p && (*p != '='))
         if (namep < nameend) *namep++ = *p++;
         else ++p;
      *namep = '\0';
      namep = strdup(name);
      if (!namep) return; // ENOMEM

      if (*p == '=') { ++p;
         valuep = value;
         while (*p && (*p != '&'))
            if (valuep < valueend) *valuep++ = *p++;
            else ++p;
         *valuep = '\0';
         valuep = urldecode(value);
         if (*p == '&') ++p;
      }
      else valuep = 0;

//fprintf(stderr, " install %s=%s$\n", namep, valuep);//DEBUG
      install(namep, valuep);
   }
}

/*
 * Decode the url-encoded string s in-place.
 * The pointer s may be NULL. Return s.
 */
static char *urldecode(char *s)
{
   register char *p, *q;
   char c, x1, x2;

   p = q = s;
   if (q) while ((c = *q++)) {
      if (c == '+') *p++ = ' ';
      else if (c == '%') {
         if (isxdigit(x1 = *q++)) {
            if (isxdigit(x2 = *q++)) {
               x1 = toupper(x1);
               if ((x1 -= '0') > 9) x1 -= 7;
               x2 = toupper(x2);
               if ((x2 -= '0') > 9) x2 -= 7;
               *p = x1 << 4; *p++ |= x2;
            }
            else { *p++ = '%'; *p++ = x1; *p++ = x2; }
         }
         else { *p++ = '%'; *p++ = x1; }
      }
      else *p++ = c;
   }
   *p++ = '\0';
   return s;
   //return p - s; // #chars after decoding
}
