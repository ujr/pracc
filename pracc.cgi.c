/* Pracc Web GUI */
/* Copyright (c) 2008 by Urs Jakob Ruetschi */

#include "cgi.h"
#include "common.h"
#include "datetools.h"
#include "pracc.h"
#include "symtab.h"

#include "ui_acct.h"
#include "ui_accts.h"
#include "ui_pclog.h"
#include "ui_pracclog.h"
#include "ui_report.h"

#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <grp.h>
#include <pwd.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#define streq(s,t) (strcmp((s),(t)) == 0)

#define TITLE "Printer Accounting" /* override in templates */

void authorise(const char *user, const char *path);
static int hasgroup(const char *user, const char *group);

int doUser(const char *suffix);
int doAdmin(const char *suffix);
int doAccounts(const char *suffix);
int doReports(const char *suffix);
int doLogs(const char *suffix);
void doLogsPracc(void);
void doLogsPagecount(void);
int doVars(const char *suffix);

int doCreateAccount(void);
int doEditAccount(void);
int doPurgeAccount(void);
int doDeleteAccount(void);

int doCredits(void);
int doAccountViewer(const char *acctname);
int doCreateChecks(void);

char *lookup(const char *name, const char *deflt);
void install(const char *name, const char *value);
void installf(const char *name, const char *format, ...);

void array_init(const char *name);
int array_load(const char *name, int index);

void array_init_acct(void);
void array_init_accts(void);
void array_init_pclog(void);
void array_init_pracclog(void);
void array_init_report(void);

char getacctinfo(const char *acctname, long *balance, long *limit);
void sendfile(const char *vn);
int parseamount(const char *s, long *value);
int parsetype(const char *s, char *type);
void instdate(struct tm *tmp);
void instint(const char *name, long value);
void initdates(const char *defltperiod);
void setdate(const char *name, time_t t);
int isCsvRequested(void);

long getfirst();
long getcount();
time_t getmintime();
time_t getmaxtime();
char *getperiod(time_t *tminp, time_t *tmaxp, const char *deflt);

static void printsym(struct symbol *sym)
{ if (sym && sym->sval) printf(" %s=%s$\n", sym->name, sym->sval); }
static void dumpsym(struct symbol *sym)
{ if (sym && sym->sval) fprintf(stderr, " %s=%s$\n", sym->name, sym->sval); }

struct symtab syms; // global symbol store
const char *myname; // basename of this program

int main(int argc, char **argv)
{
   const char *user;
   const char *path;
   const char *s;

   /* Initialise */

   myname = progname(argv);
   if (!myname) return 127;

   syminit(&syms, 128);
   cgiInit(PRACCDOC);

   user = lookup("REMOTE_USER", 0);
   path = lookup("PATH_INFO", "/");
   authorise(user, path);

   /* Branch depending on section */

   if (s = cgiPathPrefix(path, "/user"))
      return doUser(s);

   if (s = cgiPathPrefix(path, "/admin"))
      return doAdmin(s);

   if (s = cgiPathPrefix(path, "/accounts"))
      return doAccounts(s);

   if (s = cgiPathPrefix(path, "/reports"))
      return doReports(s);

   if (s = cgiPathPrefix(path, "/logs"))
      return doLogs(s);

   if (s = cgiPathPrefix(path, "/vars"))
      return doVars(s);

   if (streq(path, "/")) {
      install("SECTION", "main");
      cgiStartHTML(TITLE);
      cgiCopyTemplate("main.tmpl");
      cgiEndHTML();
   }
   else sendfile(path);

   return 0; // OK
}

/*
 * Authorisation: check if the given user is allowed
 * on the given path; return if so, exit if not.
 *
 * This function assumes that user was authenticated.
 * As a side effect, the user's role is installed
 * in the symbol table as ROLE=user|peek|poke.
 */
void authorise(const char *user, const char *path)
{
   int canpeek, canpoke;

   /* Style file and images are always allowed */

   if (streq(path, "/style.css")) return;
   if (cgiPathPrefix(path, "/images")) return;

   /* Require authentication */

   if (!user) {
      cgiStartHTML(TITLE);
      cgiError("You are not authenticated, sorry!");
      cgiEndHTML();
      exit(0);
   }

   /*
    * Authorisation:
    * everybody is allowd to pracc.cgi/user
    * group PRACCPOKE required for pracc.cgi/admin
    * group PRACCPEEK required for everything else
    */

   canpoke = hasgroup(user, PRACCPOKE);
   canpeek = hasgroup(user, PRACCPEEK) || canpoke;

   install("ROLE", canpoke ? "poke" : canpeek ? "peek" : "user");

   if (cgiPathPrefix(path, "/user")) {
      return; // OK
   }
   else if (cgiPathPrefix(path, "/admin")) {
      if (canpoke) return; // OK
   }
   else { // any other path requires PRACCPEEK
      if (canpeek) return; // OK
   }

   /* Not authorised: complain and quit */

   cgiCopyTemplate("accessdenied.tmpl");
   exit(0);
}

static int hasgroup(const char *user, const char *group)
{
   struct group *gr;
   char **pp;

   assert(user && user[0]);
   assert(group && group[0]);

   if ((gr = getgrnam(group)) == NULL) {
      cgiStartHTML(TITLE);
      cgiError("Function getgrnam(%s) failed.", group);
      cgiEndHTML();
      exit(0);
   }

   for (pp = gr->gr_mem; *pp; pp++)
      if (streq(*pp, user))
         return 1; // user in group

   return 0; // user not in group
}

/*
 * Handle pracc.cgi/user and user.tmpl;
 *
 * EVERYBODY is authorised to go to this page, so we must make
 * sure they watch only their own account(s) using praccGrant().
 * (Not yet implemented because only personal account, not group
 * accounts, can be viewed at this time.)
 */
int doUser(const char *suffix)
{
   const char *acctname;
   char *types;
   char class;

   install("SECTION", "user");

   assert(acctname = lookup("REMOTE_USER", 0));
   class = getacctinfo(acctname, 0, 0);

   initdates("thismonth");

   if (lookup("types", 0) == 0)
      install("types", ""); // all types
   
   cgiStartHTML(TITLE);
   if (class) cgiCopyTemplate("user.tmpl");
   else cgiError("No accounting for %s?", acctname);
   cgiEndHTML();

   return 0; // OK
}

/*
 * Handle pracc.cgi/admin and admin.tmpl
 *
 * All functions that change anything to the pracc system are
 * grouped here so that it is easy to require special rights.
 *
 *   op=create/edit/purge/delete/credits/checks
 *
 * If there is no valid op specified, show the admin.tmpl page.
 */
int doAdmin(const char *suffix)
{
   const char *op = lookup("op", "");

   install("SECTION", "admin");

   if (streq(op, "create"))
      return doCreateAccount();

   if (streq(op, "credits"))
      return doCredits();

   if (streq(op, "edit"))
      return doEditAccount();

   if (streq(op, "purge"))
      return doPurgeAccount();

   if (streq(op, "delete"))
      return doDeleteAccount();

   if (streq(op, "checks"))
      return doCreateChecks();

   cgiStartHTML(TITLE);
   cgiCopyTemplate("admin.tmpl");
   cgiEndHTML();

   return 0; // OK
}

int doAccounts(const char *suffix)
{
   install("SECTION", "accounts");

   /* Account specified? */
   if (suffix && (suffix[0] == '/') && suffix[1]) {
      const char *acctname = &suffix[1];

      if (getacctinfo(acctname, 0, 0))
            doAccountViewer(acctname);
      else {
         cgiStartHTML(TITLE); errno = 0;
         cgiError("%s: No such account", acctname);
         cgiEndHTML();
      }
      return 0; // OK
   }

   /* List all accounts */
   if (isCsvRequested()) {
      cgiStartCSV("Accounts.csv");
      if (accts_dump(stdout, lookup("filter", 0)) < 0)
         printf("ERROR: %s\n", strerror(errno));
   }
   else {
      cgiStartHTML(TITLE);
      cgiCopyTemplate("accounts.tmpl");
      cgiEndHTML();
   }

   return 0; // OK
}

int doReports(const char *suffix)
{
   install("SECTION", "reports");

   initdates("thismonth");

   if (isCsvRequested()) {
      time_t tmin = getmintime();
      time_t tmax = getmaxtime();
      const char *acctlist = lookup("acctlist", 0);

      cgiStartCSV("PraccReport.csv");
      if (report_dump(stdout, tmin, tmax, acctlist) < 0)
         printf("ERROR: %s\n", strerror(errno));
   }
   else {
      cgiStartHTML(TITLE);
      cgiCopyTemplate("reports.tmpl");
      cgiEndHTML();
   }

   return 0; // OK
}

int doLogs(const char *suffix)
{ 
   install("SECTION", "logs");

   if (streq(suffix, "/pracc.log"))
      cgiCopyVerbatim(PRACCLOG, "text/plain");
   else if (streq(suffix, "/pc.log"))
      cgiCopyVerbatim(PRACCPCLOG, "text/plain");
   else if (streq(suffix, "/pracc")) {
      doLogsPracc();
   }
   else if (streq(suffix, "/pclog")) {
      doLogsPagecount();
   }
   else { // default
      cgiStartHTML(TITLE);
      cgiCopyTemplate("logs.tmpl");
      cgiEndHTML();
   }
   return 0; // OK
}

void doLogsPracc()
{
   if (isCsvRequested()) {
      time_t tmin, tmax;
      char *filter, *types;

      initdates("all");
      tmin = getmintime();
      tmax = getmaxtime();

      filter = lookup("filter", 0);
      types = lookup("types", 0);

      cgiStartCSV("Log.csv");
      if (pracclog_dump(stdout, tmin, tmax, filter, types))
         printf("ERROR: %s\n", strerror(errno));
   }
   else {
      cgiStartHTML(TITLE);
      cgiCopyTemplate("pracclog.tmpl");
      cgiEndHTML();
   }
}

void doLogsPagecount()
{
   if (isCsvRequested()) {
      time_t tmin, tmax;
      const char *filter;

      initdates("all");
      tmin = getmintime();
      tmax = getmaxtime();
      filter = lookup("filter", 0);

      cgiStartCSV("pclog.csv");
      if (pclog_dump(stdout, tmin, tmax, filter) < 0)
         printf("ERROR: %s\n", strerror(errno));
   }
   else {
      cgiStartHTML(TITLE);
      cgiCopyTemplate("pclog.tmpl");
      cgiEndHTML();
   }
}

int doVars(const char *suffix)
{
   install("SECTION", "vars");

   cgiStartHTML(TITLE);
   cgiCopyTemplate("vars.tmpl");
   cgiEndHTML();
}

int doCreateAccount(void)
{
   char *value, *limit, *acct;
   long thevalue, thelimit;
   int badacct, badvalue, badlimit;
   char *who, *comment;

   acct = lookup("acct", 0);
   badacct = acct && (praccCheckName(acct) != 0);
   install("badacct", (badacct) ? "" : 0);

   value = lookup("value", 0);
   badvalue = value && !parseamount(value, &thevalue);
   install("badvalue", (badvalue) ? "" : 0);

   limit = lookup("limit", 0);
   if (limit && streq(limit, "none")) {
      thelimit = UNLIMITED;
      badlimit = 0;
   }
   else badlimit = limit && !parseamount(limit, &thelimit);
   install("badlimit", (badlimit) ? "" : 0);

   if (acct && !badacct && value && !badvalue && limit && !badlimit) {
      mode_t oldmask = umask(0117);

      comment = lookup("comment", 0);
      who = lookup("REMOTE_USER", lookup("USER", "?"));
      if (praccCreate(acct, thevalue, thelimit, who, comment, 0) == 0) {
         char buf[MAXLINE], s[16];

         /* Add log record */
         if (thelimit == UNLIMITED) strcpy(s, "none");
         else snprintf(s, sizeof(s), "%ld", thelimit);
         snprintf(buf, sizeof(buf), "init %ld limit %s", thevalue, s);
         (void) praccLogup(who, acct, buf);

         install("status", "ok");
      }
      else {
         install("status", "fail");
         install("error", strerror(errno));
      }
      umask(oldmask);

      /* Reset fields */
      install("value", 0);
      install("limit", 0);
      install("comment", 0);
   }
   else install("status", 0);

   cgiStartHTML(TITLE);
   cgiCopyTemplate("create.tmpl");
   cgiEndHTML();
   
   return 0; // OK
}

int doEditAccount(void)
{
   char *acct, *type, *value, *comment;
   int badacct, badtype, badvalue, badcomment;
   char thetype = 0;
   long thevalue;
   int valid, confirmed;

   acct = lookup("acct", 0);
   badacct = acct && !getacctinfo(acct, 0, 0);
   install("badacct", (badacct) ? "" : 0);

   type = lookup("type", 0);
   badtype = type && !parsetype(type, &thetype);
   install("badtype", (badtype) ? "" : 0);

   value = lookup("value", 0);
   if (value && strchr("+-=$", thetype)) {
      if ((thetype == '$') && streq(value, "none")) {
         thevalue = UNLIMITED;
         badvalue = 0;
      }
      else if (parseamount(value, &thevalue)) {
         if ((thevalue < 0) && ((thetype == '+') || (thetype == '-')))
            badvalue = 1;
         else badvalue = 0;
      }
      else badvalue = 1;
   }
   else badvalue = 0; // no value or type in {error,note}
   install("badvalue", (badvalue) ? "" : 0);

   comment = lookup("comment", 0); // mandatory for note and error
   badcomment = !((thetype != '#' && thetype != '!') || (comment && *comment));
   install("badcomment", (badcomment) ? "" : 0);

   valid = acct && !badacct && type && !badtype && !badvalue && !badcomment;
   confirmed = streq(lookup("confirm", ""), "yes");
   install("confirm", (valid && !confirmed) ? "" : 0);

   if (valid && confirmed) {
      char *who = lookup("REMOTE_USER", lookup("USER", "?"));

      if (praccAppend(acct/*name*/, thetype, thevalue, who, comment) == 0) {
         char buf[MAXLINE], s[16];
         char *ts = praccTypeString(thetype);

         /* Add log record */
         if ((thetype == '+') || (thetype == '-') || (thetype == '='))
            snprintf(buf, sizeof(buf), "%s %ld", ts, thevalue);
         else if (thetype == '$') {
            if (thevalue == UNLIMITED) strcpy(s, "none");
            else snprintf(s, sizeof(s), "%ld", thevalue);
            snprintf(buf, sizeof(buf), "%s %s", ts, s);
         }
         else snprintf(buf, sizeof(buf), "%s added", ts);
         (void) praccLogup(who, acct/*name*/, buf);

         install("status", "ok");
      }
      else {
         install("status", "fail");
         install("error", (errno) ? strerror(errno) : "error");
      }
   }
   else install("status", 0);

   cgiStartHTML(TITLE);
   cgiCopyTemplate("acctedit.tmpl");
   cgiEndHTML();

   return 0; // OK
}

int doPurgeAccount(void)
{
   cgiStartHTML(TITLE);
   errno = 0;
   cgiError("Sorry, the purge function is not yet implemented.");
   cgiEndHTML();

   return 0; // OK
}

int doDeleteAccount(void)
{
   char *acctname;
   char *who, *comment;
   long balance, limit;

   acctname = lookup("acct", "");
   if (*acctname && streq(lookup("confirm", ""), "yes")) {
      comment = lookup("comment", 0);
      who = lookup("REMOTE_USER", lookup("USER", "?"));

      if ((praccSum(acctname, &balance, &limit, 0, 0, 0) == 0)
          && (praccDelete(acctname) == 0)) {
         char buf[MAXLINE], s[16];

         /* Add log record */
         if (limit == UNLIMITED) strcpy(s, "none");
         else snprintf(s, sizeof(s), "%ld", limit);
         snprintf(buf, sizeof(buf), "delete balance=%ld limit=%s %s",
                    balance, s, comment ? comment : "");
         (void) praccLogup(who, acctname, buf);

         install("status", "ok");
      }
      else {
         install("status", "fail");
         install("error", (errno) ? strerror(errno) : "error");
      }
   }
   else install("status", 0);

   cgiStartHTML(TITLE);
   cgiCopyTemplate("acctkill.tmpl");
   cgiEndHTML();

   return 0; // OK
}

/* Gutschriften */
int doCredits(void)
{
   char *acct, *value;
   char class;
   long thevalue;
   int badacct, badvalue;
   int valid, confirmed;

   acct = lookup("acct", 0);
   class = getacctinfo(acct, 0, 0);
   badacct = acct && (class == 0);
   install("badacct", (badacct) ? "" : 0);

   value = lookup("value", 0);
   badvalue = value && (!parseamount(value, &thevalue) || (thevalue < 0));
   install("badvalue", (badvalue) ? "" : 0);

   valid = acct && !badacct && value && !badvalue;
   confirmed = streq(lookup("confirm", ""), "yes");
   install("confirm", (valid && !confirmed) ? "" : 0);

   if (valid && confirmed) {
      long balance;
      char *who = lookup("REMOTE_USER", lookup("USER", "?"));
      char *comment = lookup("comment", 0);

      if (praccAppend(acct, '+', thevalue, who, comment) == 0) {
         char buf[MAXLINE];

         /* Get new balance */
         getacctinfo(acct, 0, 0);

         /* Add log record */
         snprintf(buf, sizeof(buf), "credit %ld", thevalue);
         (void) praccLogup(who, acct, buf);

         install("status", "ok");
      }
      else {
         install("status", "fail");
         install("error", (errno) ? strerror(errno) : "error");
      }

      /* Reset fields */
      install("value", 0);
      install("comment", 0);
      install("confirm", 0);
      install("gecos", 0);
   }
   else install("status", 0);

   cgiStartHTML(TITLE);
   cgiCopyTemplate("credits.tmpl");
   cgiEndHTML();

   return 0; // OK
}

int doAccountViewer(const char *acctname)
{
   time_t tmin, tmax;
   const char *period;

   assert(acctname);

   period = lookup("period", "");
   if (streq(period, "")) {
      if (parsedate(lookup("tmin", ""), &tmin)); else tmin = -1;
      if (parsedate(lookup("tmax", ""), &tmax)); else tmax = -1;
      if (tmax >= 0) tmax += 86399; // 23:59:59
   }
   else daterange(period, &tmin, &tmax);

   if ((tmin < 0) || (tmax < 0))
      daterange("month", &tmin, &tmax);

   setdate("tmin", tmin);
   setdate("tmax", tmax);

   (void) getacctinfo(acctname, 0, 0);

   cgiStartHTML(TITLE);
   cgiCopyTemplate("acctview.tmpl");
   cgiEndHTML();

   return 0; // OK
}

int doCreateChecks(void)
{
   cgiStartHTML(TITLE);
   errno = 0;
   cgiError("Sorry, checks are not yet implemented.");
   cgiEndHTML();

   return 0; // OK
}

/*
 * Install a name=value pair in the global symbol store.
 * ALL symbol definitions MUST GO THROUGH THIS FUNCTION.
 *
 * Beware of the case sym->sval == value, which happens
 * if we do sth like
 *   install("name", value);
 *   install("name", lookup("name", ...));
 * In this case, free() would invalidate value and the
 * following strdup() would be meaningless!
 */
void install(const char *name, const char *value)
{
   struct symbol *sym;

   sym = symput(&syms, name);
   if (sym) {
      if (sym->sval != value) {
         if (sym->sval) free((void *) sym->sval);
         if (value) value = strdup(value);
         sym->sval = value;
      }
      else /* see notes above */ ;
   }
   else cgiError("Symbol install failed");
}

void installf(const char *name, const char *format, ...)
{
   va_list ap;
   char buf[256];

   assert(name != NULL);
   assert(format != NULL);

   va_start(ap, format);

   vsnprintf(buf, sizeof(buf), format, ap);

   install(name, buf);

   va_end(ap);
}

char *lookup(const char *name, const char *deflt)
{
   struct symbol *sym;

   if (!name) return (char *) deflt;

   sym = symget(&syms, name);
   if (sym) return (sym->sval) ? (char *) sym->sval : (char *) deflt;

   if (streq(name, "symtab")) symdump(&syms, stdout);
   if (streq(name, "symdump")) symeach(&syms, printsym);

   return (char *) deflt;
}

/*
 * Prepare the named array for subsequent calls to arrayLoad.
 * Know arrays are:
 *  accts -- list of all accounts
 *  pclog -- entries in the pc.log pagecount log
 *  pracclog -- entries in the pracc.log
 *  acct -- entries in a single account
 *  report -- list with info on selected accounts
 */
void array_init(const char *name)
{
   if (streq(name, "accts")) {
      array_init_accts();
   }
   else if (streq(name, "pclog")) {
      array_init_pclog();
   }
   else if (streq(name, "pracclog")) {
      array_init_pracclog();
   }
   else if (streq(name, "acct")) {
      array_init_acct();
   }
   else if (streq(name, "report")) {
      array_init_report();
   }
}

void array_init_accts()
{
   long first = getfirst();
   long count = getcount();
   char *filter = lookup("filter", "");
   
   // Implicit asterisk on non-empty filter:
   //if (filter[0] && !strpbrk(filter, "*?")) {
   //   snprintf(str, sizeof(str), "%s*", filter);
   //   filter = str;
   //}
   if (accts_init(first, count, filter) < 0)
      install("error", strerror(errno));
   else {
      long n = accts_count();
      installf("NR", "%ld", n); // TODO capital NR (template!)
   }
}

void array_init_pclog()
{
   long first = getfirst();
   long count = getcount();
   time_t tmin, tmax;
   const char *filter;

   initdates("all");
   tmin = getmintime();
   tmax = getmaxtime();

   filter = lookup("filter", "");
   if (!filter[0]) filter = 0;

   if (pclog_init(first, count, tmin, tmax, filter) < 0)
      install("error", strerror(errno));
   else {
      long pages, jobs;
      long n = pclog_count();
      pclog_totals(&pages, &jobs);

      installf("pclog.totpages", "%ld", pages);
      installf("pclog.totcount", "%ld", jobs);
      installf("NR", "%ld", n); // TODO capital NR (templates!)
   }
}

void array_init_pracclog()
{
   long first = getfirst();
   long count = getcount();
   time_t tmin, tmax;
   const char *filter;
   const char *types; // i d c r l n p x

   initdates("thismonth");
   tmin = getmintime();
   tmax = getmaxtime();

   filter = lookup("filter", 0);
   types = lookup("types", 0);

   if (pracclog_init(first, count, tmin, tmax, filter, types) < 0)
      install("error", strerror(errno));
   else {
      long n = pracclog_count(); //pclog_count();
      installf("NR", "%ld", n); // TODO capital NR (templates!)
   }
}

void array_init_acct()
{
   long first = getfirst();
   long count = getcount();
   time_t tmin, tmax;
   const char *acct;
   const char *types;

   initdates("thismonth");
   tmin = getmintime();
   tmax = getmaxtime();

   acct = lookup("acct", 0);
   types = lookup("types", 0);

   if (acct_init(first, count, acct, tmin, tmax, types) < 0)
      install("error", strerror(errno));
   else {
      long n = acct_count();
      installf("NR", "%ld", n); // TODO capital NR (templates!)
   }
}

void array_init_report()
{
   long first = getfirst();
   long count = getcount();
   time_t tmin, tmax;
   const char *acctlist;

   initdates("lastyear");
   tmin = getmintime();
   tmax = getmaxtime();

   acctlist = lookup("acctlist", 0);

   if (report_init(first, count, tmin, tmax, acctlist) < 0)
      install("error", strerror(errno));
   else {
      long credits, debits;
      long n = report_count();
      report_totals(&credits, &debits);
      instint("report.totcredits", credits);
      instint("report.totdebits", debits);
      installf("NR", "%ld", n); // TODO capital NR (templates!)
   }
}

/*
 * Load the given index of the named array
 * into the global symbol table.
 *
 * Return 0 if ok, 1 if no such index,
 * and -1 on any other error (with errno set).
 */
int array_load(const char *name, int index)
{
   char buf[32];
   int r;

   if (streq(name, "accts")) {
      char *acct;

      r = accts_get(index, &acct);
      if (r == 0) getacctinfo(acct, 0, 0);
   }
   else if (streq(name, "pclog")) {
      char *name;
      time_t t;
      long pc, pages, jobs;

      r = pclog_get(index, &name, &t, &pc, &pages, &jobs);
      if (r == 0) {
         install("printer", name);
         instint("pc", pc);
         instint("pages", pages);
         instint("jobs", jobs);

         instdate(localtime(&t));
      }
   }
   else if (streq(name, "pracclog")) {
      time_t t;
      char *user, *acct, *info;

      r = pracclog_get(0, &t, &user, &acct, &info);
      if (r == 0) {
         instdate(localtime(&t));
         install("user", user);
         install("acct", acct);
         install("info", info);
         // type = i d c r l n p x   TODO
      }
      else if (r < 0)
         install("error", strerror(errno));
      else {
         long n = pracclog_count();
         instint("NR", n);
      }
   }
   else if (streq(name, "acct")) {
      time_t t;
      char type, *user, *comment;
      long value;

      r = acct_get(0, &t, &type, &value, &user, &comment);
      if (r == 0) {
         sprintf(buf, "%c", type);
         install("type", buf);
         if ((type == '$') && (value == UNLIMITED))
            snprintf(buf, sizeof(buf), "%s", "none");
         else if ((type == '#') || (type == '!'))
            buf[0] = '\0'; // no value for note/error
         else sprintf(buf, "%ld", value);
         install("value", buf);
         instdate(localtime(&t));
         install("user", user);
         install("comment", comment);
         instint("NR", acct_count());
      }
      else if (r < 0)
         install("error", strerror(errno));
      else {
         long n = acct_count();
         instint("NR", n);
      }
   }
   else if (streq(name, "report")) {
      struct acctinfo *aip;
      char class;

      r = report_get(index, &aip);
      if (r == 0) {
         class = praccAccountInfo(aip->acct, buf, sizeof(buf));
         if (class) {
            install("acct", aip->acct);
            install("gecos", (buf[0]) ? buf : 0);
            sprintf(buf, "%c", class);
            install("class", buf);
            instint("balance", aip->balance);
            if (aip->limit == UNLIMITED) {
               install("limit", "none");
               install("diff", "999999999");
            }
            else {
               instint("limit", aip->limit);
               instint("diff", aip->balance - aip->limit);
            }
            instint("credits", aip->credits);
            instint("debits", aip->debits);
            instdate(localtime(&aip->lastused));
         }
         else r = -1;
      }
      else if (r < 0)
         install("error", strerror(errno));
      else {
         // install totals
      }
   }
   else r = 1;

   return r;
}

void sendfile(const char *vn)
{
   char path[1024];
   char *type = cgiGuessMimeType(vn);
   char *prefix = PRACCDOC;
   struct stat stbuf;

   assert(vn);

   if (vn[0] == '/') ++vn;
   snprintf(path, sizeof(path), "%s/%s", prefix, vn);
   if (stat(path, &stbuf) == 0) {
      if (S_ISDIR(stbuf.st_mode)) errno = EISDIR;
      else { // try sending the file
         cgiCopyVerbatim(path, type);
         return;
      }
   }
   cgiStartHTML(TITLE);
   cgiError("Requested operation cannot be performed");
   cgiEndHTML();
}

/*
 * Add account information to the global symbol store:
 *  acct, class, gecos, balance, limit;
 *  year, month, day, time (of last use);
 *  diff (=balance-limit, 999999999 if unlimited).
 * If bp is not NULL, store the account's balance there.
 * If lp is not NULL, store the account's limit there.
 *
 * Return the account class, 0 if no such account.
 */
char getacctinfo(const char *acctname, long *bp, long *lp)
{
   long balance, limit;
   time_t lastused;
   char buf[128], class;

   if (!acctname) return 0;

   class = praccAccountInfo(acctname, buf, sizeof(buf));
   if (!class) return class; // no such account or error

   install("acct", acctname);
   install("gecos", (buf[0]) ? buf : 0);
   sprintf(buf, "%c", class);
   install("class", buf);

   if (praccSum(acctname, &balance, &limit, 0, 0, &lastused))
      install("error", strerror(errno));
   else {
      struct tm *tmp;
      snprintf(buf, sizeof(buf), "%ld", balance);
      install("balance", buf);
      if (limit == UNLIMITED) {
         install("limit", "none");
         install("diff", "999999999");
      }
      else {
         instint("limit", limit);
         instint("diff", balance - limit);
      }
      instdate(localtime(&lastused));

      if (bp) *bp = balance;
      if (lp) *lp = limit;
   }
   return class;
}

/*
 * Install year, month, day, time in the global
 * symbol table, based on the given struct tm.
 * Month and day with a leading zero of < 10.
 * Time in "military" HHMM format.
 */
void instdate(struct tm *tmp)
{
   char buf[32];

   if (tmp) {
      sprintf(buf, "%d", 1900+tmp->tm_year);
      install("year", buf);
      sprintf(buf, "%02d", 1+tmp->tm_mon);
      install("month", buf);
      sprintf(buf, "%02d", tmp->tm_mday);
      install("day", buf);
      sprintf(buf, "%04d", 100*tmp->tm_hour + tmp->tm_min);
      install("time", buf);
   }
   else {
      install("year", 0);
      install("month", 0);
      install("day", 0);
      install("time", 0);
   }
}

/*
 * Install name=value where value is a long integer
 * that will be rendered as a decimal integer.
 */
void instint(const char *name, long value)
{
   char buf[16];

   assert(name);

   snprintf(buf, sizeof(buf), "%ld", value);
   install(name, buf);
}

/*
 * Parse date in yyyy-mm-dd format into a time_t.
 * Return number of characters scanned, 0 on error.
 */
//int parsedate(const char *s, time_t *tp)
//{
//   struct tm tm;
//   time_t t;
//   int n;
//
//   if (!s) return 0;
//   if (!(n = scandate(s, &tm))) return 0;
//
//   tm.tm_sec = tm.tm_min = tm.tm_hour = 0;
//   if ((t = mktime(&tm)) < 0) return 0;
//
//   if (tp) *tp = t;
//
//   return n; // #chars scanned
//}

/*
 * Parse an amount, which is required to be one of these forms:
 * N or N.M where N is any integer (also negative) and M is one
 * or two decimal digits. Store 100*N+M in the value pointer.
 * Return the number of characters parsed or 0 on error.
 */
int parseamount(const char *s, long *value)
{
   const char *p = s;
   long N = 0, M = 0;
   int n, sign = 1;

   if (*p == '-') { sign = -1; ++p; }
   else if (*p == '+') ++p; // skip

   if (n = scanu(p, &N)) p += n;
   else return 0;

   if (*p == '.') {
      if ((n = scanu(++p, &M)) && (n < 3)) p += n;
      else return 0;
   }

   if (*p) return 0;
  
   if (value) *value = sign * (100*N + M);

   return p - s; // #chars parsed
}

/*
 * Parse the given string for an account record type.
 * Return the number of characters parsed or 0 on error.
 */
int parsetype(const char *s, char *type)
{
   if (!s || !*s) return 0;

   if (streq(s, "credit")) {
      if (type) *type = '+';
      return 6;
   }
   if (streq(s, "debit")) {
      if (type) *type = '-';
      return 5;
   }
   if (streq(s, "reset")) {
      if (type) *type = '=';
      return 5;
   }
   if (streq(s, "limit")) {
      if (type) *type = '$';
      return 5;
   }
   if (streq(s, "error")) {
      if (type) *type = '!';
      return 5;
   }
   if (streq(s, "note")) {
      if (type) *type = '#';
      return 4;
   }
   return 0;
}

void initdates(const char *defltperiod)
{
   time_t tmin, tmax;
   const char *period;

   period = getperiod(&tmin, &tmax, defltperiod);

   setdate("tmin", tmin);
   setdate("tmax", tmax);
   install("period", period);
}

void setdate(const char *name, time_t t)
{
   char buf[32];
   struct tm *tmp;

   assert(name);

   if (!(tmp = localtime(&t))) strcpy(buf, "?");
   else strftime(buf, sizeof(buf), "%Y-%m-%d", tmp);
   install(name, buf);
}

int isCsvRequested()
{
   return !strcasecmp(lookup("format", ""), "CSV");
}

/* Symbol table lookup utilities */

long getfirst()
{
   char *s = lookup("first", "");
   if (s[0]) {
      long first = atol(s);
      if (first < 1) return 1;
      return first;
   }
   return 1; // default
}

long getcount(long deflt)
{
   char *s = lookup("count", "");
   if (s[0]) {
      long count = atol(s);
      if (count < 0) return 0;
      return count;
   }
   return 99999; // default
}

time_t getmintime()
{
   time_t tmin;

   if (parsedate(lookup("tmin", 0), &tmin))
      return tmin;

   return -1; // no time
}

time_t getmaxtime()
{
   time_t tmax;

   if (parsedate(lookup("tmax", 0), &tmax))
      return tmax + 86399; // 23:59:59, end of day

   return -1; // no time
}

char *getperiod(time_t *tminp, time_t *tmaxp, const char *deflt)
{
   time_t tmin, tmax;
   const char *period = lookup("period", "");

   // Get explicity dates or date period:
   if (streq(period, "")) {
      tmin = getmintime();
      tmax = getmaxtime();
   }
   else daterange(period, &tmin, &tmax);

   // Apply default period if no dates yet:
   if (((tmin < 0) || (tmax < 0)) && deflt)
      daterange(period=deflt, &tmin, &tmax);

   if (tminp) *tminp = tmin;
   if (tmaxp) *tmaxp = tmax;

   return (char *) period;
}
