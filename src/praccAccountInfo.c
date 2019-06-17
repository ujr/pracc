#include "pracc.h"

#include <ctype.h>
#include <errno.h>
#include <grp.h>
#include <pwd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define streq(s,t) (strcmp(s,t) == 0)

/**
 * Get information about an account: its type and
 * the owner's full name (= user's gecos field).
 *
 * Return an indicator of the type of the account:
 *   P  a personal account,
 *   G  a group account,
 *   D  the default account, or
 *   z  zombie account (no owner).
 *
 * Return 0 if there is an error (errno has the details)
 * or if the named account does not exist.
 */
char
praccAccountInfo(const char *acctname, char *buf, int len)
{
   struct passwd *pw;
   struct group *gr;
   const char *path;
   struct stat stbuf;

   /* Start with an empty owner's name */
   if (buf && (len > 0)) buf[0] = '\0';

   /* Valid account name? */
   if (!acctname || praccCheckName(acctname)) {
      errno = EINVAL;
      return 0;
   }

   /* See if account exists */
   path = praccPath(acctname);
   if (!path) return 0; // ENOMEM
   if (stat(path, &stbuf)) return 0;

   /* Is it the default account? */
   if (streq(acctname, PRACCDEFLT)) {
      if (buf) snprintf(buf, len, "default account");
      return 'D'; // default account
   }

   /* Is it a personal account? */
   if ((pw = getpwnam(acctname))) {
      if (buf) snprintf(buf, len, "%s", pw->pw_gecos);
      return 'P'; // personal account
   }

   /* Is it a group account? */
   if ((gr = getgrnam(acctname)))
      return 'G'; // group account

   return 'z'; // zombie account
}

#ifdef TESTING
#include <errno.h>
#include <stdio.h>
int main(int argc, char **argv)
{
   const char *me = *argv++;
   char buf[80];
   while (*argv) {
      char type = praccAccountInfo(*argv, buf, sizeof(buf));
      if (type) printf("%c %s %s\n", type, *argv++, buf);
      else printf("%s: %s\n", *argv++, strerror(errno));
   }
   return 0;
}
#endif /* TESTING */
