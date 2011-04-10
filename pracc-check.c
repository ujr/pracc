/* pracc-check.c - a utility in the pracc package
 * Copyright (c) 2007-2008 by Urs Jakob Ruetschi
 */

/***
 d 2770 PRACCOWNER:PRACCGROUP PRACCDIR/             required
 r  660 PRACCOWNER:PRACCGROUP PRACCDIR/PRACCDEFLT   required
 r  660 PRACCOWNER:PRACCGROUP PRACCDIR/USER         optional
 r  660 PRACCOWNER:PRACCGROUP PRACCLOG              required
 r  660 PRACCOWNER:PRACCGROUP PRACCPCLOG            optional
 d  ??? *:*                   PRACCBIN              required
 r  755 *:*                   PRACCBIN/TOOL         required
 r 2755 PRACCOWNER:PRACCGROUP PRACCBIN/pracc-credit (not yet)
 d  755 *:*                   PRACCDOC/             required
 d  ??? *:*                   PRACCCGI/pracc.cgi    required
***/

#include <assert.h>
#include <dirent.h>
#include <errno.h>
#include <grp.h>
#include <pwd.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "common.h"
#include "pracc.h"
#include "print.h"
#include "streq.h"

int check(char *home, char *sub, char *fn,
          int type, int uid, int gid, int mode, int opt);
void grumble(char *home, char *sub, char *fn, char *fmt, ...);
char *strftype(int mode);
char *strfperm(int mode);
int checkfile(const char *fn);
void usage(const char *s);

char *me;
int grumbled = 0;

int main(int argc, char **argv)
{
   struct group *gr;
   gid_t praccgid;

   struct passwd *pw;
   uid_t praccuid;

   DIR *dir;
   struct dirent *d;

   long goodcnt = 0;
   long badcnt = 0;

   int c;

   extern int optind;
   extern int otperr;

   me = progname(argv);
   if (!me) return 127;

   opterr = 0; // prevent stupid getopt output
   while ((c = getopt(argc, argv, "hV")) > 0) switch (c) {
      case 'h': usage(0); // show help
      case 'V': return praccIdentify("pracc-check");
      default:  usage("invalid option");
   }
   argc -= optind;
   argv += optind;

   if (*argv) {
      const char *fn;
      const char *acctname = *argv++;
      if (praccCheckName(acctname))
         die(111, "invalid account name");
      if ((fn = praccPath(acctname)) == 0)
         die(111, "praccPath failed");
      return checkfile(fn);
   }
   
   if (*argv) usage("too many arguments");

   putfmt(stdout, "Compile-time configuration:\n");
   putfmt(stdout, "PRACCOWNER = %s (owner for pracc files)\n", PRACCOWNER);
   putfmt(stdout, "PRACCGROUP = %s (group for pracc files)\n", PRACCGROUP);
   putfmt(stdout, "PRACCDEFLT = %s (default account)\n", PRACCDEFLT);
   putfmt(stdout, "PRACCDIR = %s (directory for pracc files)\n", PRACCDIR);
   putfmt(stdout, "PRACCLOG = %s (the pracc accounting log)\n", PRACCLOG);
   putfmt(stdout, "PRACCPCLOG = %s (the pracc pagecount log)\n", PRACCPCLOG);
   putfmt(stdout, "PRACCBIN = %s (directory for pracc tools)\n", PRACCBIN);
   putfmt(stdout, "PRACCDOC = %s (directory for pracc docs)\n", PRACCDOC);
   putfmt(stdout, "PRACCCGI = %s (directory for pracc.cgi)\n", PRACCCGI);
   putfmt(stdout, "PRACCPEEK = %s (pracc observer group, r/o)\n", PRACCPEEK);
   putfmt(stdout, "PRACCPOKE = %s (pracc admin group, r/w)\n", PRACCPOKE);

   /* Ensure minimal lengths for MAXNAME and MAXLINE */
   assert(MAXNAME > 9);  // max account name length
   assert(MAXLINE > 79); // max pracc file line length

   gr = getgrnam(PRACCGROUP);
   if (!gr) die(127, "getgrnam %s failed", PRACCGROUP);
   praccgid = gr->gr_gid;

   pw = getpwnam(PRACCOWNER);
   if (!pw) die(127, "getpwnam %s failed", PRACCOWNER);
   praccuid = pw->pw_uid;

   gr = getgrnam(PRACCPEEK);
   if (!pw) die(127, "getgrnam %s failed", PRACCPEEK);
   gr = getgrnam(PRACCPOKE);
   if (!pw) die(127, "getgrnam %s failed", PRACCPOKE);

   putln(stdout, "\nChecking web interface...");
   check(0, 0, PRACCDOC, S_IFDIR, -1, -1, 0755, 0);
   check(0, 0, PRACCCGI, S_IFDIR, -1, -1, -1, 0);
   check(PRACCCGI, 0, "pracc.cgi", S_IFREG, -1, -1, -1, 0);

   putln(stdout, "\nChecking log files...");
   check(0, 0, PRACCLOG, S_IFREG, praccuid, praccgid, 0660, 0);
   check(0, 0, PRACCPCLOG, S_IFREG, praccuid, praccgid, 0660, 1);

   putln(stdout, "\nChecking pracc tools...");
   check(0, 0, PRACCBIN, S_IFDIR, -1, -1, -1, 1);
   check(PRACCBIN, 0, "pracc-init", S_IFREG, -1, -1, 0755, 0);
   check(PRACCBIN, 0, "pracc-edit", S_IFREG, -1, -1, 0755, 0);
   check(PRACCBIN, 0, "pracc-view", S_IFREG, -1, -1, 0755, 0);
   check(PRACCBIN, 0, "pracc-kill", S_IFREG, -1, -1, 0755, 0);
   check(PRACCBIN, 0, "pracc-sum", S_IFREG, -1, -1, 0755, 0);
#if 0 /* pracc-credit is not yet implemented */
   check(PRACCBIN, 0, "pracc-credit", S_IFREG, 0, praccgid, 02755, 1);
#endif
#if 0 /* pracc-purge is not yet implemented */
   check(PRACCBIN, 0, "pracc-purge", S_IFREG, -1, -1, 0755, 0);
#endif
   check(PRACCBIN, 0, "pracc-check", S_IFREG, -1, -1, 0755, 0);
   check(PRACCBIN, 0, "pracc-log", S_IFREG, -1, -1, 0755, 0);
   check(PRACCBIN, 0, "pracc-ok", S_IFREG, -1, -1, 0755, 1); // legacy
   
   putln(stdout, "\nChecking pracc files...");
   check(0, 0, PRACCDIR, S_IFDIR, praccuid, praccgid, 02770, 0);
   if (check(PRACCDIR, 0, PRACCDEFLT, S_IFREG, praccuid, praccgid, 0660, 0))
      ++badcnt; else ++goodcnt;
   dir = opendir(PRACCDIR);
   if (!dir) die(127, "cannot opendir %s", PRACCDIR);
   while (d = readdir(dir)) {
      char *fn = d->d_name;
      if (streq(fn, ".")) continue;
      if (streq(fn, "..")) continue;
      if (praccCheckName(fn)) {
         grumble(PRACCDIR, 0, fn, "invalid pracc name");
         ++badcnt;
         continue;
      }
      if (check(PRACCDIR, 0, fn, S_IFREG, praccuid, praccgid, 0660, 0)) {
         ++badcnt;
         continue;
      }
      ++goodcnt; // if we made it to here, the file was good
   }
   closedir(dir);
   putfmt(stdout, "There were %d+%d good+bad files in %s\n",
          goodcnt, badcnt, PRACCDIR);
   
   return (badcnt) ? 1 : 0;
}

int check(home,sub,fn,type,uid,gid,mode,opt)
char *home, *sub, *fn;
int type, uid, gid, mode, opt;
{
   struct stat st;
   struct passwd *pw;
   struct group *gr;

   grumbled = 0; // assume success

   if (home && (chdir(home) < 0)) {
      if (opt) grumble(home,sub,fn,"does not exist");
      else die(127, "cannot chdir %s", home);
   }
   if (sub && (chdir(sub) < 0)) {
      if (opt) grumble(home,sub,fn,"does not exist");
      else die(127, "cannot chdir %s/%s", home, sub);
   }

   if ((uid != -1) && !(pw = getpwuid(uid)))
      die(127, "getpwuid %d failed", uid);
   if ((gid != -1) && !(gr = getgrgid(gid)))
      die(127, "getgrgid %d failed", gid);

   if (stat(fn, &st) < 0) {
      if (errno == ENOENT) {
         if (!opt) grumble(home,sub,fn,"does not exist");
         else grumble(home,sub,fn,"optional file does not exist");
      }
      else die(127, "stat .../%s failed", fn);
      return 0; // bad
   }

   if ((uid != -1) && (uid != st.st_uid))
      grumble(home,sub,fn,"wrong owner (should be %s #%d)", pw->pw_name, uid);
   if ((gid != -1) && (gid != st.st_gid))
      grumble(home,sub,fn,"wrong group (should be %s #%d)", gr->gr_name, gid);
   if ((mode != -1) && (mode != (st.st_mode & 07777)))
      grumble(home,sub,fn,"wrong permissions (should be %s)", strfperm(mode));
   if (type != (st.st_mode & S_IFMT))
      grumble(home,sub,fn,"wrong type (should be %s)", strftype(type));

   return grumbled ? -1 : 0; // bad or good
}

void grumble(char *home, char *sub, char *fn, char *fmt, ...)
{
   char *p, *end;
   char buf[256];

   va_list ap;
   va_start(ap, fmt);

   p = buf;
   end = buf + sizeof(buf) - 9;
   if (home) {
      p += printsn(p, home, end-p);
      p += printc(p, '/');
   }
   if (sub) {
      p += printsn(p, home, end-p);
      p += printc(p, '/');
   }
   if (fn) {
      p += printsn(p, fn, end-p);
      p += printc(p, ':');
      p += printc(p, ' ');
   }
   p += vsnprintf(p, end-p, fmt, ap);
   p += printc(p, '\n');
   
   putbuf(stdout, buf, p-buf);

   va_end(ap);

   grumbled = 1;
}

char *strftype(int mode)
{
   switch (mode & S_IFMT) {
   case S_IFREG: return "regular file";
   case S_IFDIR: return "directory";
   case S_IFLNK: return "symbolic link";
   case S_IFIFO: return "named pipe";
   default: abort(); /* not here */
   }
}

char *strfperm(int mode)
{
   static char buf[8];
   register char *p;

   mode &= 07777;
   
   p = buf + sizeof(buf);
   *--p = '\0';
   do {
      *--p = '0' + (mode % 8);
      mode /= 8;
   }
   while (mode > 0);

   return p;
}

/*
 * Carefully check the specified pracc file.
 * Return 0 if valid, 111 on syntax errors.
 */
int checkfile(const char *fn)
{
   // TODO (see praccRead for inspiration)
   fprintf(stderr, "Not yet implemented, sorry.\n");
   return 127;
}

void usage(const char *err)
{
   FILE *fp = (err) ? stderr : stdout;
   if (err) putfmt(stderr, "%s: %s\n", me, err);
   putfmt(fp, "Usage: %s [account]\n", me);
   putfmt(fp, "Check the basic pracc installation.\n");
   putfmt(fp, " account: check this account\n");
   exit((err) ? 127 : 0); // FAILURE or OK
}
