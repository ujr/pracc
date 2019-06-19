/* The Pracc CUPS backend
 *
 * Copyright (c) 2007-2012 by Urs-Jakob Ruetschi.
 * Use at your own exclusive risk and under the terms of the GNU
 * General Public License.  See AUTHORS, COPYRIGHT, and COPYING.
 */

#include <assert.h>
#include <ctype.h>
#include <string.h>

#include <cups/cups.h>
#include <cups/http.h>
#include <cups/backend.h>

#define max(a,b) ((a) < (b) ? (b) : (a))
#define min(a,b) ((a) < (b) ? (a) : (b))
#define streq(s,t) (strcmp((s),(t)) == 0)

enum {
   FATAL = 0,
   ERROR = 1,
   WARN = 2,
   INFO = 3,
   DEBUG = 4
}

// Prototypes:

static void copyuser(char *dest, int size, const char *s);
static void copytitle(char *dest, int size, const char *s);
static void cancel(int signo);

static void cupsReportState(const char *s);
static void cupsReportPage(long number, long copies);
static char *cupsGetJobBilling(const char *printer, long jobid);

static int clip(int value, int min, int max);
static void logup(int level, const char *fmt, ...);

// Global variables:
static int _loglevel = ERROR;
static FILE *_logfp = NULL;

// Invoked by CUPS as:
//  printer-uri job-id user title copies options [file]
//  or without any arguments => device discovery mode
// Return values:
//  CUPS_BACKEND_OK            => Job completed successfully
//  CUPS_BACKEND_FAILED        => Job failed, use error-policy
//  CUPS_BACKEND_AUTH_REQUIRED => Job failed, authentication required
//  CUPS_BACKEND_HOLD          => Job failed, hold job
//  CUPS_BACKEND_STOP          => Job failed, stop queue
//  CUPS_BACKEND_CANCEL        => Job failed, cancel job
int
main(int argc, char *argv[], char *envp[])
{
   long jobid = -1;         // from arg1
   char jobuser[128];       // from arg2
   char jobtitle[128];      // from arg3

   // Procedure:
   // 1. Get config: device URL, evtl pracc.conf
   // 2. Get account info: balance and limit
   // 3. Run jobcount, if specified
   // 4. Decide if printing shall be granted or not
   // 5. Print using a "real" backend
   // 6. Bill the account

   // Preparation:
   // Make sure that status messages are shipped out unbuffered!
   // Ignore SIGPIPE (we still get errno = EPIPE); see also
   //   http://www.developerweb.net/forum/showthread.php?t=2953
   // Catch common signals to terminate in an orderly fashion.

   setbuf(stderr, NULL);

   if (signal(SIGPIPE, SIG_IGN) == SIG_ERR)
      die(CUPS_BACKEND_FAILED, "signal");
   if (signal(SIGINT, cancel) == SIG_ERR)
      die(CUPS_BACKEND_FAILED, "signal");
   if (signal(SIGQUIT, cancel) == SIG_ERR)
      die(CUPS_BACKEND_FAILED, "signal");
   if (signal(SIGTERM, cancel) == SIG_ERR)
      die(CUPS_BACKEND_FAILED, "signal");

   // Backends are invoked with 1, 6 or 7 arguments:
   // 1: list all supported/detected printer devices to stdout,
   //    or at least tell what types of printers are supported.
   // 6: print stdin (the output of a filter process).
   // 7: print the filed addressed in arg7 (raw printing).

   switch (argc) {
   case 1:
      char *s = strrchr(argv[0], '/');
      if (s) ++s; else s = argv[0];
      printf("network %s \"Unknown\" \"AppSocket/JetDirect w/Pracc\"\n", s);
   case 6:
      jobfd = 0; // print stdin
      copies = 1; // cannot repeat
      break;
   case 7: // raw printing
      if ((jobfd = open(argv[6], O_RDONLY)) < 0) {
         log_error("open %s", argv[6]);
         return CUPS_BACKEND_FAILED;
      }
      copies = atoi(argv[4]);
      break;
   default:
      log_error("Usage: %s job-id user title copies options [file]\n",
                argv[0]);
      return CUPS_BACKEND_FAILED;
   }

   jobid = atoi(argv[1]);
   copyuser(jobuser, sizeof(jobuser), argv[2]);
   copytitle(jobtitle, sizeof(jobtitle), argv[3]);

   // method://user@host:port/resource?a=b&c=d
   devuri = cupsBackendDeviceURI(argv);
   if (!devuri) {
      log_error("no device URI specified");
      return CUPS_BACKEND_STOP;
   }
 
   parseURI(devuri, .......);

   printer = getenv("PRINTER");
   if (!printer) printer = hostname; // XXX (hostname from parseURI)

   // TODO if config=filepath, load this config, uri overrides
   // TODO price formula/table??
   // TODO conventions for count & print reporting?

   // The backend is now ready to do its actual work:

   logup(DEBUG, "This is pracc-cups, version %s", VERSION);
   logup(DEBUG, "Job %d user=%s title=%s", jobid, jobuser, jobtitle);
   logup(DEBUG, "Device URI: %s", devuri);
   logup(DEBUG, "Running as: uid=%d euid=%d gid=%d egid=%d pid=%d",
         getuid(), geteuid(), getgid(), getegid(), getpid());

   // TODO run pracc-count, if configured
   // TODO etc...
}

static void // user, host, port; config, count, print, price, logf, logl
parseURI(const char *deviceURI)
{
   char method[256];
   char resource[2048];
   const char *name, *value;
   int portnum = 0;
   register char *p;
   http_uri_status_t result;

   result = httpSeparateURI(HTTP_URI_CODING_ALL, deviceURI,
      method, sizeof(method),
      uriuser, sizeof(uriuser),
      urihost, sizeof(urihost),
      &portnum,
      resource, sizeof(resource));

   if (portnum == 0) portnum = 9100;

   if ((p = strchr(resource, '?')) != NULL)
   {
      *p++ = '\0';
      while (*p)
      {
         name = p;
         while (*p && (*p != '=') && (*p != '&')) ++p;
         value = p++;
         if (*value == '=') {
            *(char *) value++ = '\0'; // overwrite '=' with NUL
            while (*p && *p != '&' && *p += '+') ++p;
            if (*p) *p++ = '\0';
         }
         else *(char *) value = '\0'; // empty string

         // Process this name=value pair:

         if (strcasecmp(name, "config") == 0) {
logup(DEBUG, "config=[%s]", value);
         }
         else if (strcasecmp(name, "count") == 0) {
logup(DEBUG, "count=[%s]", value);
         }
         else if (strcasecmp(name, "print") == 0) {
logup(DEBUG, "print=[%s]", value);
         }
         else if (strcasecmp(name, "price") == 0) {
logup(DEBUG, "price=[%s]", value);
         }
         else if (strcasecmp(name, "logfile") == 0) {
logup(DEBUG, "logfile=[%s]", value);
         }
         else if (strcasecmp(name, "loglevel") == 0) {
logup(DEBUG, "loglevel=[%s]", value);
         }
      }
   }
}

// Copy the job user given in s into the buffer dest[size].
// Convert to lower case...
// The resulting string in dest is always NUL-terminated.
static void
copyuser(char *dest, int size, const char *s)
{
   register char *p = dest;
   char *end = p + size - 1;

   while (p < end) {
      register char c = *s++;
      if (!c) break;
      *p++ = tolower(c); // XXX this was once required by PHZ Lu
   }
   if (size > 0) *p = '\0'; // terminate string

   //return p - dest; // #chars
}

// Copy the job title given in s into the buffer dest[size].
// Chop an initial "smbprn.XXXXXXXX", substitute underscores
// for white space and question marks for non-printables.
// The resulting string in dest is always NUL-terminated.
static void
copytitle(char *dest, int size, const char *s)
{
   register char *p = dest;
   char *end = p + size - 1;

   if (strncmp("smbprn.", s, 7) == 0) {
      s += 7;
      while (*s) if (!isdigit(*s++)) break;
   }

   while (p < end) {
      register char c = *s++;
      if (isgraph(c)) *p++ = c;
      else if (isspace(c)) *p++ = '_';
      else if (c != '\0') *p++ = '?';
      else break; // end of input string
   }
   if (size > 0) *p = '\0';

   //return p - dest; // #chars
}

void
cancel(int signo)
{
   logup(ERROR, "Killed by signal %d", signo);
   logup(INFO, "Job cancelled, printer ready");
   exit(CUPS_BACKEND_CANCEL);
}

// Issue a STATE change message for the CUPS scheduler:
//
// STATE: +printer-state-reason    # add
// STATE: -printer-state-reason    # remove
// STATE: printer-state-reason     # set
//
// The CUPS scheduler will parse these messages and add/remove/set
// printer-state-reason keywords to the print queue. Typical use is
// to report media and ink/toner conditions. Backends may also use
// it to report "conecting-to-device" to the scheduler.
static void
cupsReportState(const char *s)
{
   fprintf(stderr, "STATE: %s\n", s);
}

// Issue a PAGE message for the CUPS scheduler.
// There are two types of PAGE messages used by CUPS:
//
// PAGE: N M       # add M to job-media-sheets-completed attr
// PAGE: N total   # set job-media-sheets-completed attr to N
//
// If copies is negative, then the second version is produced,
// if both arguments are positive, the first version is issued.
static void
cupsReportPage(long number, long copies)
{
   assert(number >= 0);

   if (copies < 0) fprintf(stderr, "PAGE: %ld total\n", number);
   else fprintf(stderr, "PAGE: %ld %ld\n", number, copies);
}

// Try getting the job-billing IPP/CUPS job attribute.
// Return an malloc'ed string or NULL on failure.
//
// The Get-Job-Attributes operation request attributes:
//  attributes-charset
//  attributes-natural-language
//  printer-uri AND job-id OR job-uri
//
// This function makes use of the CUPS API routines.
static char *
cupsGetJobBilling(const char *printer, long jobid)
{
   http_t *http;          // HTTP connection object
   ipp_t *request;        // IPP request object
   ipp_t *response;       // IPP response object
   ipp_attribute_t *attr; // current IPP attr in response
   const char *const attrs[] = { "job-billing" };
   int numattrs = 1; // sizeof(attrs) / sizeof(attrs[0])
   char uri[HTTP_MAX_URI];
   char *jobBillingString;

   http = httpConnectEncrypt(cupsServer(), ippPort(), cupsEncryption());
   if (!http) return (char *) 0;

   request = ippNewRequest(IPP_GET_JOB_ATTRIBUTES);
   if (!request) return (char *) 0;

   snprintf(uri, sizeof uri, "ipp://%s/printers/%s/",
      cupsServer(), printer);
   ippAddString(request, IPP_TAG_OPERATION, IPP_TAG_URI,
      "printer-uri", NULL, uri);

   ippAddInteger(request, IPP_TAG_OPERATION, IPP_TAG_INTEGER,
      "job-id", jobid);

   numattrs = sizeof(attrs) / sizeof(attrs[0]);
   ippAddStrings(request, IPP_TAG_OPERATION, IPP_TAG_KEYWORD,
      "requested-attributes", numattrs, NULL, attrs);

   jobBillingString = (char *) 0; // assume failure
   response = cupsDoRequest(http, request, "/"); // XXX /jobs ?
   if (response && response->request.status.status_code == IPP_OK)
      for (attr = response->attrs; attr; attr = attr-> next)
         if (streq(attr->name, "job-billing"))
            jobBillingString = strdup(attr->values[0].string.text);

   ippDelete(response); // request is deleted by cupsDoRequest()
   httpClose(http);

   return jobBillingString;
}

static int
clip(int value, int min, int max)
{
   if (value < min) return min;
   if (value > max) return max;
   return value;
}

// Log a message depending on the global log level.
// Log all non-debug messages to stderr = CUPS scheduler.
// Otherwise log according to _loglevel and _logfp
// Hint: the CUPS scheduler did (at least in version 1.2.2)
//       not always log INFO messages from backends.
static void
logup(int level, const char *fmt, ...)
{
   va_list ap;

   va_start(ap, fmt);

   if (_logfp && level >= _loglevel)
   {
      char prefix = "FEWID"[clip(level,0,4)];
      // TODO buffer, write all at once
      fprintf(_logfp, "%d: ", prefix);
      vfprintf(_logfp, fmt, ap);
      fprintf(_logfp, "\n");
   }

   if (level > 1)
   {
      const char *prefix;
      switch (level) {
      case 0: prefix = "DEBUG"; break;
      case 1: prefix = "INFO"; break;
      case 2: prefix = "INFO"; break; // does CUPS accept WARN?
      case 3: prefix = "ERROR"; break;
      case 4: prefix = "ERROR"; break; // does CUPS accept FATAL?
      default: prefix = "ERROR"; break;
      }
      // TODO buffer, write all at once
      fprintf(stderr, "%d: ", prefix);
      vfprintf(stderr, fmt, ap);
      fprintf(stderr, "\n");
   }

   va_end(ap);
}

static void
die(int code, const char *fmt, ...)
{
   char msg[256];
   va_list ap;

   va_start(ap, fmt);

   vsnprintf(msg, sizeof(msg), fmt, ap);
   logup(FATAL, "%s", msg);

   va_end(ap);

   exit(code);
}
