#include <ctype.h>
#include "scan.h"

/*
 * Match the string s against the pattern pat and return
 * the number of chars matched or 0 if s and pat do not match.
 *
 * Pattern:
 * 1. any sequence of blanks at the beginning of pat match any
 *    sequence of white space, even the empty sequence, in s;
 * 2. any other sequence of blanks match any non-empty sequence
 *    of white space in s;
 * 3. an asterisk matches any character up to the first occurrence
 *    of the next character of pat in s;
 * 4. every other character matches itself.
 *
 * There is no pat that requires a blank at the beginning of s.
 * If you need that, say: if (isspace(*p) && (n=scanpat(p, " ...")))
 */

int scanpat(const char *s, const char *pat)
{
   const char *sp = s; // source pointer
   const char *pp = pat; // pattern pointer

   for (;;) {
      register char c = *pp++;
      if (c == '\0') break;
      if (c == ' ') { // blank: sequence of white space
         const char *q = sp;
         while ((*sp == ' ') || (*sp == '\t')) ++sp;
         if ((sp == q) && (pp > pat+1)) return 0;
         while (isspace(*pp)) ++pp;
         continue;
      }
      if (c == '*') { // star: anything up to next char in pat
         c = *pp;
         while (*sp && *sp != c) ++sp;
         if (c == '\0') break;
         continue;
      }
      if (c != *sp++) return 0; // mismatch
   }
   return sp - s; // #chars matched
}

#ifdef TEST
#include <stdio.h>
int main(int argc, char *argv[])
{
  int i, n;
  char *pat = argv[1]; /* first arg is pattern */
  for (i = 2; i < argc; i++) {
  	n = scanpat(argv[i], pat);
   	printf("%s: %d\n", argv[i], n);
  }
  return 0;
}
#endif
