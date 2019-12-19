#include <assert.h>
#include <errno.h>
#include <pwd.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "pracc.h"
#include "print.h"
#include "putln.h"

#define COMMENT "by pracc-purge"

static char *mkline(struct praccbuf *pracc);
static char *mklimit(long limit, time_t tstamp, const char *username);
static char *mkreset(long balance, time_t tstamp, const char *username);

int praccPurge(acctname, tx, keepresets, keeplimits, keepnotes, doit, fntmp)
   const char *acctname;
   time_t tx; // cutoff time
   int keepresets;
   int keeplimits;
   int keepnotes;
   int doit;
   const char *fntmp;
{
   struct passwd *pw;
   const char *username;
   const char *fn;  // name of pracc file to be purged
   time_t tr;       // current record's time stamp
   FILE *fptmp;     // temporary file pointer
   struct praccbuf pracc;
   long limit, balance;
   int n, breakdone = 0;

   assert(acctname && fntmp);

   pw = getpwuid(getuid());
   if (pw == 0) return -1;
   username = pw->pw_name;

   umask(0007); // owner and group only
   if ((remove(fntmp) < 0) && (errno != ENOENT)) return -1;
   if ((fptmp = fopen(fntmp, "w")) == 0) return -1;

   if (praccOpen(acctname, &pracc) < 0) return -1;
   if ((fn = strdup(pracc.fn)) == 0) return -1;

/*
 * Once here, we have a pracc reader and an open temporary
 * file pointer. Now iterate the pracc file and decide if
 * a record shall be copied to the temporary file...
 */

   balance = limit = 0;

   while ((n = praccRead(&pracc)) > 0) {
      tr = pracc.tstamp;

      if (tr == 0) putln(fptmp, mkline(&pracc));
      else if (tr < tx) switch (pracc.type) {
         case '=': if (keepresets) putln(fptmp, mkline(&pracc)); break;
         case '$': if (keeplimits) putln(fptmp, mkline(&pracc)); break;
         case '#': if (keepnotes) putln(fptmp, mkline(&pracc)); break;
      }

/*
 * At the transition from skipping to keeping records,
 * include the current account balance and limit!
 */

      else {
         if (!breakdone) {
            if (!keeplimits)
               putln(fptmp, mklimit(limit, tx, username));
            if (!keepresets)
               putln(fptmp, mkreset(balance, tx, username));
            breakdone = 1;
         }
         putln(fptmp, mkline(&pracc));
      }

   }

/*
 * Ensure there are limit and reset records in the purged account
 * EVEN if all existing records are older than the break date!
 */

   if (tr < tx) {
      if (!keeplimits)
         putln(fptmp, mklimit(limit, tx, username));
      if (!keepresets)
         putln(fptmp, mkreset(balance, tx, username));
   }

/*
 * Ensure there were no errors writing to the temporary file.
 * Close both the pracc original file and the temporary file.
 * Then move the temporary file _over_ the original...
 */

   if (praccClose(&pracc) < 0) return -1;
   if (ferror(fptmp) != 0) return -1;
   if (fclose(fptmp) != 0) return -1;

   if (doit && (rename(fntmp, fn) < 0)) return -1;

   return 0; // SUCCESS
}

static char *mkline(struct praccbuf *pracc)
{
   static char buf[MAXLINE];
   praccAssemble(buf, pracc->type, pracc->value, pracc->tstamp,
                 pracc->username, pracc->comment);
   return buf; // XXX ptr to static var!
}

static char *mklimit(long limit, time_t tstamp, const char *username)
{
   static char buf[MAXLINE];
   praccAssemble(buf, '$', limit, tstamp, username, COMMENT);
   return buf;
}

static char *mkreset(long balance, time_t tstamp, const char *username)
{
   static char buf[MAXLINE];
   praccAssemble(buf, '=', balance, tstamp, username, COMMENT);
   return buf;
}

