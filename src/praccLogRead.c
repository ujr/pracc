#include "pracc.h"

#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>

#include "getln.h"
#include "scan.h"
#include "tai.h"

int praccLogOpen(struct pracclog *logentry)
{
   assert(logentry);

   logentry->lineno = 0;
   logentry->fp = fopen(PRACCLOG, "r");
   if (!logentry->fp) return -1;

   return 0; // SUCCESS
}

int praccLogRead(struct pracclog *logentry)
{
   char line[MAXLINE];
   char *userp, *acctp;
   register char *p;
   struct tai tai;
   int n, skipped;

   assert(logentry && logentry->fp);
reread: p = line;
   n = getln(logentry->fp, line, sizeof(line), &skipped);
   if (n > 0) {
      // Format: @STAMP by USER acct NAME: INFO
      logentry->lineno += 1;
      line[n-1] = '\0';

      p += scanpat(p, " "); // skip space

      if ((n = taiscan(p, &tai))) p += n;
      else goto reread; // skip bad line

      if ((n = scanpat(p, " by "))) p += n;
      else goto reread; // skip bad line

      userp = p;
      while (*p && !isspace(*p)) ++p;
      if (*p) *p++ = '\0';
      else goto reread; // skip bad line

      if ((n = scanpat(p, "  acct "))) p += n;
      else goto reread; // skip bad line

      acctp = p;
      while (*p && (*p != ':')) ++p;
      if (*p) *p++ = '\0';
      else goto reread; // skip bad line

      while (isspace(*p)) ++p;

      if (scanpat(p, "init ")) logentry->type = 'i';
      else if (scanpat(p, "credit ")) logentry->type = 'c';
      else if (scanpat(p, "debit ")) logentry->type = 'd';
      else if (scanpat(p, "reset ")) logentry->type = 'r';
      else if (scanpat(p, "limit ")) logentry->type = 'l';
      else if (scanpat(p, "note ")) logentry->type = 'n';
      else if (scanpat(p, "purge ")) logentry->type = 'p';
      else if (scanpat(p, "delete ")) logentry->type = 'x';
      else if (scanpat(p, "deleted ")) logentry->type = 'x';
      else if (scanpat(p, "error ")) logentry->type = 'e';
      else logentry->type = '?';

      logentry->tstamp = taiunix(&tai);
      strncpy(logentry->username, userp, MAXNAME);
      strncpy(logentry->acctname, acctp, MAXNAME);
      strncpy(logentry->infostr, p, MAXLINE);

      return 1; // SUCCESS
   }

   return (n < 0) ? -1 : 0; // error or eof
}

int praccLogClose(struct pracclog *logentry)
{
   assert(logentry);

   if (logentry->fp) {
      fclose(logentry->fp);
      logentry->fp = 0;
   }
   logentry->lineno = 0;
   return 0; // SUCCESS
}

