/* praccapi.c - implementation of pracc API functions */
/* Copyright (c) 2008 by Urs Jakob Ruetschi */

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <grp.h>
#include <pwd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#include "hasgroup.h"
#include "pracc.h"
#include "print.h"
#include "tai.h"

#define min(x,y) (((x) < (y)) ? (x) : (y))

int praccDelete(const char *acctname, const char *comment);
int praccPurge(const char *acctname, struct tai *date,
	int keepresets, int keeplimits, int keepnotes, int doit);

#if 0
int praccSum(const char *acctname, long *balp, long *limp)
{
   struct praccbuf pracc;
   long balance, limit, n;

   if (praccOpen(acctname, &pracc) < 0) return -1;

   balance = limit = 0;
   while ((n = praccRead(&pracc)) > 0) {
      long value = pracc.value;
      switch (pracc.type) {
         case '-': // debit
            balance -= value;
            break;
         case '+': // credit
            balance += value;
            break;
         case '=': // reset
            balance = value;
            break;
         case '$': // limit
            limit = value;
            break;
      }
   }
   (void) praccClose(&pracc);
   if (balp) *balp = balance;
   if (limp) *limp = limit;
   return (n < 0) ? -1 : 0;
}
#endif

#if 0
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

   praccClose(pracc);
   pracc->lineno = 0;
   pracc->fn = fn;
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
   n = getln(pracc->fp, line, sizeof(line));
   if (n > 0) {
      if (line[n-1] != '\n')
         skipline(pracc->fp);
      pracc->lineno += 1;
//putfmt(stdout, "\n{%d} ", pracc->lineno);
//putln(stdout, line); //XXX DEBUG

      pracc->type = (int) *p++;
      pracc->value = 0;
      tainull(&pracc->taistamp);
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
            goto next;
         default: // skip
            goto next;
      }

      p += scanpat(p, " ");
      if (n = taiscan(p, &tai)) p += n;
      else return 1; // ok but incomplete
      pracc->taistamp = tai;

      p += scanpat(p, " ");
      for (i = 0; *p && (i < MAXNAME); i++)
         if (isspace(*p)) break;
         else pracc->username[i] = *p++;
      pracc->username[min(i,MAXNAME-1)] = '\0';

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
#endif

#if 0
/*
 * Check if access to a pracc file should be granted or denied.
 * Pracc files have owner:group = root:pracc, so ordinary users
 * cannot access their pracc files other than through tools.
 *
 * Tools that run set-uid root or set-gid pracc must make sure
 * that users access pracc files in an orderly manner, that is:
 *
 *  1. each user can access his/her personal account
 *  2. each user can access group account X iff he/she is in group X
 *  3. tools must make sure users do not tamper with their accounts
 *
 * The praccGrant() routine ensures rules 1 and 2.
 * It returns 0 if access should be granted, 1 if access should be
 * denied, and -1 if there was a system error (in which case errno
 * gives details about the error and access should be denied).
 *
 * Note: this routine returns 1 (deny access) if it is running
 * set-uid or set-gid and the given username does not match the
 * real uid. Otherwise, it is assumed that the username argument
 * was authenticated by the calling instance!
 */

#define DENY 1
#define GRANT 0
#define ERROR -1

int praccGrant(const char *username, const char *acctname)
{
   struct passwd *pw;
   struct group *pgr;
   uid_t useruid;
   gid_t praccgid;
   int inpracc;
   char **pp;
   
   pgr = getgrnam(PRACCGROUP);
   if (!pgr) return ERROR;
   praccgid = pgr->gr_gid;
   inpracc = hasgroup(praccgid);

   /*
    * Pracc files are accessible only to root and group pracc,
    * so we check if the effective uid is root or if the effective
    * gid is pracc or if one of the auxiliary gids is pracc.
    */

   if ((geteuid() != 0) && (getegid() != praccgid) && !inpracc) {
      errno = EPERM; // operation not permitted
      return ERROR;
   }

   /*
    * Check the given username for plausibility: it must
    * exist in the system user database and, if we are
    * running set-uid or set-gid, also match the real uid.
    */

   pw = getpwnam((char *) username);
   if (!pw) switch (errno) {
      case 0:
      case ENOENT:
      case ESRCH:
         errno = 0;
         return DENY; // invalid user
      default:
         return ERROR;
   }

#ifdef DEBUG
   fprintf(stderr, "DEBUG: praccGrant: requesting user:%s #%d\n",
           username, pw->pw_uid);
   fprintf(stderr, "DEBUG: praccGrant: requested account: %s\n", acctname);
#endif

   useruid = pw->pw_uid;
   if ((getuid() != geteuid()) || (getgid() != getegid())) {
      if (useruid != getuid()) return DENY;
   }

   /*
    * Ok, we are technically able to access pracc files and
    * the given username looks plausible. Now check if the
    * account to be accessed is the user's personal account
    * or a group account to which the user has access.
    */

   if (strcmp(username, acctname) == 0) return GRANT; // personal account

   pgr = getgrnam(acctname);
   if (!pgr) return DENY; // not ERROR
   for (pp = pgr->gr_mem; *pp; pp++) {
      if (strcmp(*pp, username) == 0) return GRANT; // group account
   }

   /*
    * The given acctname corresponds neither to the given user's
    * personal account nor to a group account of which the user
    * is a member. Therefore, access should be denied.
    */

   errno = 0;
   return DENY;
}
#endif

#if 0
/*
 * Return a pointer to an malloc'ed string containing
 * the full path to the pracc file for the given acctname
 * or NULL on error.
 *
 * Note: This is the place to make changes if it is ever
 * decided that pracc files should be kept in a hashed
 * directory structure: /var/pracc/a/asmith, etc.
 */
char *praccPath(const char *acctname)
{
   int n = strlen(PRACCDIR);
   int m = strlen(acctname);

   char *path = (char *) calloc(n+1+m+1, sizeof(char));
   if (path == (char *) 0) return (char *) 0; // ENOMEM

   strcpy(path, PRACCDIR);
   if (path[n-1] != '/') path[n++] = '/';
   strcpy(path+n, acctname);

   return path;
}
#endif

#if 0
/*
 * Check if a given string is a valid pracc account name.
 * Pracc account names consist of at most MAXNAME printable
 * characters, excluding blanks, slashes, and backslashes.
 *
 * Return 0 if valid, -1 if invalid.
 */
int praccCheckName(const char *acctname)
{
   register const char *p;

   errno = EINVAL;
   if (!acctname || !acctname[0]) return -1;
   for (p = acctname; *p; p++) {
      if (!isprint(*p)) return -1;
      if (isblank(*p)) return -1;
      if (*p == '/') return -1;
      if (*p == '\\') return -1;
   }
   if (p - acctname > MAXNAME) return -1; // too long

   errno = 0;
   return 0; // acctname valid
}
#endif

#if 0
/*
 * Append a line to the common pracc log. Line format:
 *
 *   @timestamp by USER acct NAME: INFO
 *
 * where "info" depends on the tool that appends the line:
 *
 *   init AMOUNT limit LIMIT [overwrite]
 *   debit AMOUNT
 *   credit AMOUNT
 *   reset BALANCE
 *   limit LIMIT
 *   note added
 *   purge to DATE OPTIONS
 */
int praccLog(const char *acctname, const char *infostr)
{
  struct passwd *pw;
  const char *username;
  char buf[MAXLINE];
  register char *p;
  const char *end;
  int fd, len;

  if (praccCheckName(acctname) < 0) abort(); // BUG

  if (pw = getpwuid(getuid())) username = pw->pw_name;
  else return -1; // failed - see errno

  p = buf;
  end = buf + sizeof(buf);

  p += taistamp(p);
  p += prints(p, " by ");
  p += fmt_user(p, username, min(MAXNAME, end-p));
  p += printsn(p, " acct ", end-p);
  p += printsn(p, acctname, min (MAXNAME, end-p));
  p += printsn(p, ": ", end-p);
  p += fmt_info(p, infostr, end-p);
  p += printc(p, '\n');
  len = p - buf;

  fd = open(PRACCLOG, O_CREAT | O_WRONLY | O_APPEND, 0660);
  if (fd < 0) return -1; // see errno

  if (write(fd, buf, len) != len) return -1;
  if (close(fd) < 0) return -1;

  return 0; // SUCCESS
}
#endif

#if 0
/*
 * Pracc does not handle usernames that contain spaces.
 * This routine simply copies the given user name up to
 * the first space into the buffer provided. Truncated
 * names are flagged with an exclamation (!) mark. This
 * routine is used by all API functions that write to
 * pracc files or the common log file.
 */
static int fmt_user(char *buf, const char *username, int size)
{
   register int c, i = 0;

   if (size <= 0) return 0;
   if (size > MAXNAME) size = MAXNAME;
   while ((c = *username++) && (i < size)) {
      if (isspace(c)) break;
      else buf[i++] = c;
   }
   if (c) { // flag truncation!
      if (i < size) buf[i] = '!';
      else buf[size-1] = '!';
   }
   return i; // #chars written to buf
}

/*
 * Copy infostr to buf up to the first naughty character.
 * Translate tabs to blanks while copying.
 */
static int fmt_info(char *buf, const char *infostr, int size)
{
   register int c, i = 0;

   if (size <= 0) return 0;
   while ((c = *infostr++) && (i < size)) {
      if (c == '\t') c = ' '; // no tabs please
      if (isgraph(c) || (c == ' ')) buf[i++] = c;
      else break;
   }
   return i; // #chars written to buf
}
#endif
