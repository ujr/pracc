/* This file is part of the "pracc" printer accounting software. */
/* Copyright (c) 2005-2007 by Urs-Jakob Ruetschi. All rights reserved */

#include <ctype.h>
#include <errno.h>

#include "pracc.h"

/*
 * Check if a given string is a valid pracc account name.
 * Pracc accounts names consist of at most MAXNAME printable
 * characters, excluding blanks, slashes,and backslashes.
 *
 * Return 0 if valid, -1 if invalid chars, -2 if too long.
 */

int praccname(const char *acctname)
{
   register const char *p;

   errno = EINVAL;
   if (!acctname || !acctname[0]) return -1; // invalid name
   for (p = acctname; *p; p++) {
      if (!isprint(*p)) return -1; // invalid name
      if (isblank(*p)) return -1;
      if (*p == '/') return -1;
      if (*p == '\\') return -1;
   }

   if (p - acctname > MAXNAME) return -2; // name too long

   errno = 0;
   return 0; // acctname is valid
}
