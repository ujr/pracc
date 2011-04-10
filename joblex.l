 /* Count pages in print jobs. Urs Jakob Ruetschi 2008 */
%{
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

const char *me;
long vpages = 0; // assumption
long copies = 1; // assumption
long lineno = 1;
int verbose = 0;

char lang[32], *langptr = lang;
const char *langend = lang + sizeof(lang) - 1;

int atend = 0;  // set to 1 on %%Pages: (atend)
int gotit = 0;  // set to 1 if pages determined

char first;     // parametrized char of a PCL seq
char group;     // group char of a PCL param seq
long value;     // value of a PCL param seq

void addlang(char l);
void skip(long nbytes);
void dopcl(int first, int group, long value, int param);
void debug(const char *fmt, ...);
%}

ESC     \x1B
FF      \x0C
WS      [ \t]
UEL     {ESC}%-12345X
NUMBER  [[:digit:]]+
ATEND   "(atend)"
ENTER   @PJL{WS}+ENTER{WS}+LANGUAGE{WS}*={WS}*

 /*
  * PCL character ranges: two-charachter escape sequences TC,
  * parametrized character PC, group character GC, parameter
  * character zC, and termination character ZC.
  */

TC      [\x30-\x7E]
PC      [\x21-\x2F]
GC      [\x60-\x7E]
zC      [\x60-\x7E]
ZC      [\x40-\x5E]
PCLNUM  [-+]?([[:digit:]]{0,5}\.)?[[:digit:]]{1,5}

%s PJL PJL_ENTER
%s PS PS_PAGES PS_PAGE
%s PCL PCL_PARAM PCL_HPGL
%s PCLXL

%%

 /*
  * Switching between languages: PJL, PS, PCL, and PCLXL.
  */

{UEL}                { BEGIN(INITIAL); addlang('U');
                       debug("saw plain UEL, enter INITIAL"); }
{UEL}/@PJL           { BEGIN(PJL); addlang('U'); addlang('J');
                       debug("saw UEL@PJL, enter PJL"); }
<INITIAL>^%!         { BEGIN(PS); addlang('P');
                       debug("saw %!, enter PS"); }
<INITIAL>^@PJL       { BEGIN(PJL); addlang('J');
                       debug("saw ^@PJL, enter PJL"); }
<INITIAL>{ESC}       { BEGIN(PCL); addlang('5');
                       debug("saw plain ESC, enter PCL"); }

 /*
  * Rules for PostScript: look for %%Pages comments and %%[Page: N]%%
  * messages (Windows PS drivers often include code that generates
  * these but omit DSC %%Pages comments).
  *
  * Looking for /#copies is dangerous without a full PostScript
  * interpreter: many drivers push the result of a calculation
  * on the stack and then define /#copies. Same for /NumCopies.
  * The %%Requirements: comment has a numcopies(N) option, but
  * I've never seen it in practice...
  */

<PS>%%Pages:{WS}*    { if (!gotit) BEGIN(PS_PAGES); }
<PS_PAGES>{ATEND}    { atend = 1; debug("saw %%%%Pages: (atend)"); }
<PS_PAGES>{NUMBER}   { long n = atol(yytext);
                       if (vpages < n) vpages = n;
                       if (!atend) gotit = 1;
                       debug("saw %%%%Pages: %d", n); }
<PS_PAGES>.|\n       { BEGIN(PS); }

<PS>\(%%\[Page:{WS}*{NUMBER}\]%%\) { if (!gotit) {
                       atend = 1; BEGIN PS_PAGE; yyless(9); }}
<PS_PAGE>{WS}*{NUMBER} { long n = atol(yytext);
                       if (vpages < n) vpages = n;
                       debug("saw (%%%%[Page: %d]%%%%)" ,n); }
<PS_PAGE>.|\n        { BEGIN PS; }
                    
 /*
  * In PJL mode, we just look for @PJL ENTER LANGUAGE
  * to determine which language follows next...
  */

<PJL>{ENTER}         { BEGIN(PJL_ENTER); }
<PJL_ENTER>POSTSCRIPT{WS}*$  { BEGIN(PS); addlang('P');
                       debug("saw PJL ENTER, enter PS"); }
<PJL_ENTER>PCL{WS}*\r?$ { BEGIN(PCL); addlang('5');
                       debug("saw PJL ENTER, enter PCL"); }
<PJL_ENTER>PCLXL{WS}*\r?$  { BEGIN(PCLXL); addlang('6');
                       debug("saw @PJL ENTER, enter PCLXL"); }
<PJL_ENTER>.*        { BEGIN(INITIAL); addlang('?');
                       debug("saw @PJL ENTER LANGUAGE = %s", yytext); }

 /*
  * In PCL mode, we completely parse the PCL stuff, assign values
  * and parameter names to global variables, and invoke dopcl()
  * for all parametrised PCL commands. The patterns that just
  * increment magic I've found in pclcount by Oliveira & Manin,
  * but cannot find any reference in the PCL specification...
  */

<PCL>{FF}            { ++vpages; debug("FF ends page %ld", vpages); }

<PCL>{ESC}%-1B       { debug("Stand-alone plotter mode (?)"); }
<PCL>{ESC}%[0-9]*B   { BEGIN PCL_HPGL; debug("enter HPGL mode"); }
<PCL_HPGL>.|\n       { /* ignore HP-GL/2 */ }
<PCL_HPGL>{ESC}%[0-9]*A { BEGIN PCL; debug("enter PCL mode (after HPGL)"); }
<PCL>{ESC}%[0-9]*A   { debug("enter PCL mode (w/o previous HPGL mode)"); }

<PCL>{ESC}           { debug("unexpected ESC at top level"); }
<PCL>{ESC}{TC}       { /* ignore PCL two-char command */
                       debug("{ESC %c}", yytext[1]); }
<PCL>{ESC}{PC}{GC}   { first = yytext[1];
                       group = yytext[2];
                       value = 0; // default
                       BEGIN PCL_PARAM; }
<PCL_PARAM>{PCLNUM}  { const char *p = yytext;
                       if ((*p == '-') || (*p == '+')) ++p;
                       value = atol(p); /* trunc float */ }
<PCL_PARAM>{zC}      { dopcl(first, group, value, yytext[0]); }
<PCL_PARAM>{ZC}      { dopcl(first, group, value, yytext[0]);
                       BEGIN(PCL); }
<PCL_PARAM>{ESC}     { yyless(0); BEGIN(PCL); debug("early ESC in param"); }
<PCL_PARAM>.         { /* ignore anything else */ }
<PCL>.               { /* ignore anything else */ }
 /*<PCL>{ESC}\*p{NUMBER}[XY]\x80{FF} { ++magic; }*/
 /*<PCL>{ESC}\*p{NUMBER}x{NUMBER}[XY]\x80{FF} { ++magic; }*/

 /* I've no idea about PCL6 (aka PCLXL) so just skip it! */

<PCLXL>.             { /* ignore PCLXL */ }

 /* Maintain lineno and ignore anything else */

\n                   { ++lineno; }
.                    { /* ignore */ }

%%

int main(int argc, char **argv)
{
   if (argv && *argv) me = *argv++;
   else return 127; // no arg0?

   if (*argv && !strcmp(*argv, "-v")) ++argv, verbose = 1;

   if (*argv) {
      const char *fn = *argv++;
      if (!(yyin = fopen(fn, "r"))) {
         fprintf(stderr, "%s: open %s: %s\n", me, fn, strerror(errno));
         return 111;
      }
   }
   else yyin = stdin;

   if (*argv) {
      fprintf(stderr, "Usage: %s [-v] [jobfile]\n", me);
      fprintf(stderr, "Read stdin if no jobfile; -v is verbose\n");
      return 127;
   }

   //yyout = fopen("/dev/null", "w");
   yylex();
   addlang(0); // terminate!

   if (verbose) fprintf(stderr, "\n");
   printf("%ld vpages=%ld copies=%ld struct=%s\n",
          vpages*copies, vpages, copies, lang);
   //if (magic > 0) printf("magic=%ld\n", magic);

   return 0; // SUCCESS
}

int yywrap(void) { return 1; }

void addlang(char l)
{
   if (l) { // append language code
      if (langptr < langend) *langptr++ = l;
      else if (langptr > lang) langptr[-1] = '!'; // too long
   }
   else *langptr = '\0'; // terminate
}

#define PCLCODE(x,y,z) (65536*(x&255) + 256*(y&255) + (z&255))

void dopcl(int first, int group, long value, int param)
{
   if (verbose) debug("{ESC %c %c %ld %c}",
      first, group, value, param);

   //if (0x60 <= param && param <= 0x7E) param -= 0x20;
   switch (PCLCODE(first,group,param)) {
   case PCLCODE('&','b','W'): // i/o config data
   case PCLCODE('&','p','X'): // transparent data
   case PCLCODE('&','n','W'): // string data
   case PCLCODE('(','f','W'): // symbol set definition
   case PCLCODE(')','s','W'): // font descriptor data
   case PCLCODE('(','s','W'): // character/descriptor data
   case PCLCODE('*','c','W'): // pattenr data
   case PCLCODE('*','b','V'): // raster data (a plane)
   case PCLCODE('*','b','W'): // raster data (a row)
   case PCLCODE('*','v','W'): // CID (Configure Image Data)
   case PCLCODE('*','m','W'): // dither matrix data
   case PCLCODE('*','l','W'): // color lookup table data
   case PCLCODE('*','i','W'): // viewing illuminant data
      debug("skipping %ld bytes of %c%c#%c data",
            value, first&255, group&255, param&255);
      skip(value);
      break;
   case PCLCODE('&','l','X'):
      copies = value;
      debug("set PCL copies = %ld", copies);
      break;
   default: // ignore
      break;
   }
}

void skip(long nbytes)
{
#if 1
   while (nbytes-- > 0) {
      int c = input();
      if (c == EOF) break;
      //fprintf(stderr, " %ld\n", nbytes);
   }
#else
   if (fseek(yyin, nbytes, SEEK_CUR) == 0) yyrestart(yyin);
   else exit(111);
#endif
}

void debug(const char *fmt, ...)
{
   va_list ap;
   va_start(ap, fmt);
   if (!verbose) return;
   fprintf(stderr, "Line %d: ", lineno);
   vfprintf(stderr, fmt, ap);
   fprintf(stderr, "\n");
   va_end(ap);
}
