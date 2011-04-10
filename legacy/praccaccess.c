/* This file is part of the "pracc" printer accounting software. */
/* Copyright (c) 2007 by Urs-Jakob Ruetschi. All rights reserved */

#include <assert.h>
#include <errno.h>
#include <grp.h>
#include <pwd.h>
#include <sys/types.h>

#define DEBUG
#include <stdio.h> //DEBUG

#include "pracc.h"

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
 * This routine is part of the pracc API and ensures rule 1 and 2.
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

int praccaccess(const char *username, const char *acctname)
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

#ifdef DEBUG
   fprintf(stderr, "DEBUG: praccaccess: PRACCGROUP = %s #%d\n",
           PRACCGROUP, praccgid);
#endif

   /*
    * Pracc files are accessible only to root and group pracc,
    * so we check if the effective uid is root or if the effective
    * gid is pracc or if one of the auxiliary gids is pracc.
    */

   inpracc = hasgroup(praccgid);

#ifdef DEBUG
   fprintf(stderr,
           "DEBUG: praccaccess: uid=%d:%d, gid=%d:%d, inpracc=%d\n",
           getuid(), geteuid(), getgid(), getegid(), inpracc);
#endif

   if ((geteuid() != 0) && (getegid() != praccgid) && !inpracc) {
      errno = EPERM; // operation not permitted
      return ERROR;
   }

   /*
    * Check the given username for plausibility: it must
    * exist in the system user database and, if we are
    * running set-uid or set-gid, match the real uid.
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
   fprintf(stderr, "DEBUG: praccaccess: requesting user: %s #%d\n",
           username, pw->pw_uid);
   fprintf(stderr, "DEBUG: praccaccess: requested account: %s\n", acctname);
#endif

   useruid = pw->pw_uid;
   if ((getuid() != geteuid()) || (getgid() != getegid())) {
      if (useruid != getuid()) return DENY; // invalid user XXX
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
    * personal account nor to a group account to which the user
    * has access => access should be denied.
    */

   errno = 0;
   return DENY;
}
