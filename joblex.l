 /* Count pages in print jobs. Urs Jakob Ruetschi 2008-2012 */
 // TODO Pass lineno to debug() statements
%{
#include <assert.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "joblex.h"

#define MAX(a,b) ((a)>(b)?(a):(b))

static long lineno = 1;
static int verbose = 0;
static struct printer *printer;

static char lang[32], *langptr = lang;
static const char *langend = lang + sizeof(lang) - 1;

static int atend = 0;  // set to 1 on %%Pages: (atend)
static int gotit = 0;  // set to 1 if pages determined

static char first;     // parametrized char of a PCL seq
static char group;     // group char of a PCL param seq
static long value;     // value of a PCL param seq

static void skip(long nbytes);
static void dopcl(int first, int group, long value, int param);
static void pjlvar(const char *text);

static int parse_int(const char *s, int len);
static int parse_bool(const char *s, int len);
static int parse_duplex(const char *s, int len);
%}

ESC     \x1B
FF      \x0C
WS      [ \t]
UEL     {ESC}%-12345X
NUMBER  [[:digit:]]+
ATEND   "(atend)"
ENTER   @PJL{WS}+ENTER{WS}+LANGUAGE{WS}*={WS}*
SET     @PJL{WS}+SET{WS}+

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

%s PJL PJL_ENTER PJL_SET
%s PS PS_PAGES PS_PAGE
%s PCL PCL_PARAM PCL_HPGL
%s PCLXL

%%

 /*
  * Switching between languages: PJL, PS, PCL, and PCLXL.
  */

{UEL}                { BEGIN(INITIAL); //addlang('U');
                       joblex_printer_set_lang(printer, 'U');
                       debug("Got plain UEL, enter INITIAL"); }
{UEL}/@PJL           { BEGIN(PJL); //addlang('U'); addlang('J');
                       joblex_printer_set_lang(printer, 'U');
                       joblex_printer_set_lang(printer, 'J');
                       debug("Got UEL@PJL, enter PJL"); }
<INITIAL>^%!         { BEGIN(PS); //addlang('P');
                       joblex_printer_set_lang(printer, 'P');
                       debug("Got %!, enter PS"); }
<INITIAL>^@PJL       { BEGIN(PJL); //addlang('J');
                       joblex_printer_set_lang(printer, 'J');
                       debug("Got ^@PJL, enter PJL"); }
<INITIAL>{ESC}       { BEGIN(PCL); //addlang('5');
                       joblex_printer_set_lang(printer, '5');
                       debug("Got plain ESC, enter PCL");
                       pcl5_parse(printer); BEGIN(INITIAL); }
                       /* TODO Ok? Will parse arbitrary binary files as PCL
                               and give immensely wrong values... */

 /*
  * PostScript: look for %%Pages comments and %%[Page: N]%%
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
<PS_PAGES>{ATEND}    { atend = 1; debug("Got %%%%Pages: (atend)"); }
<PS_PAGES>{NUMBER}   { long n = atol(yytext);
                       if (printer->pages < n) printer->pages = n;
                       if (!atend) gotit = 1;
                       debug("Got %%%%Pages: %d", n); }
<PS_PAGES>.|\n       { BEGIN(PS); }

<PS>\(%%\[Page:{WS}*{NUMBER}\]%%\) { if (!gotit) {
                       atend = 1; BEGIN PS_PAGE; yyless(9); }}
<PS_PAGE>{WS}*{NUMBER} { long n = atol(yytext);
                       if (printer->pages < n) printer->pages = n;
                       debug("Got (%%%%[Page: %d]%%%%)", n); }
<PS_PAGE>.|\n        { BEGIN PS; }
                    
 /*
  * In PJL mode, we look for @PJL ENTER LANGUAGE
  * and interesting @PJL SET name = value stuff
  */

<PJL>{ENTER}         { BEGIN(PJL_ENTER); }
<PJL_ENTER>POSTSCRIPT{WS}*$  { BEGIN(PS); //addlang('P');
                       joblex_printer_set_lang(printer, 'P');
                       debug("Got PJL ENTER, enter PS"); }
<PJL_ENTER>PCL{WS}*\r?$ { BEGIN(PCL); //addlang('5');
                       joblex_printer_set_lang(printer, '5');
                       debug("Got PJL ENTER, enter PCL");
                       pcl5_parse(printer, stderr, verbose);
                       BEGIN(PJL); // back to PJL (pcl5_parse stops after UEL)
                     }
<PJL_ENTER>PCLXL{WS}*\r?$  { BEGIN(PCLXL); //addlang('6');
                       joblex_printer_set_lang(printer, '6');
                       debug("Got @PJL ENTER, enter PCLXL");
                       pclxl_parse(printer, stderr, verbose);
                     }
<PJL_ENTER>.*        { BEGIN(INITIAL); //addlang('?');
                       joblex_printer_set_lang(printer, '?');
                       debug("Got @PJL ENTER LANGUAGE = %s", yytext); }
<PJL>{SET}           { BEGIN(PJL_SET); }
<PJL_SET>.*\r?       { BEGIN(PJL); pjlvar(yytext); }

 /*
  * In PCL mode, we completely parse the PCL stuff, assign values
  * and parameter names to global variables, and invoke dopcl()
  * for all parametrised PCL commands. The patterns that just
  * increment magic I've found in pclcount by Oliveira & Manin,
  * but cannot find any reference in the PCL specification...
  */

<PCL>{FF}            { debug("Got PCL FF (page eject)");
                       printer->pages += printer->copies;
                       printer->sheets += printer->duplex
                          ? (float) printer->copies/2 : printer->copies; }
<PCL>{ESC}%-1B       { debug("Stand-alone plotter mode (?)"); }
<PCL>{ESC}%[0-9]*B   { BEGIN(PCL_HPGL); debug("Enter HPGL mode"); }
<PCL_HPGL>.|\n       { /* ignore HP-GL/2 */ }
<PCL_HPGL>{ESC}%[0-9]*A { BEGIN(PCL); debug("Enter PCL mode (after HPGL)"); }
<PCL>{ESC}%[0-9]*A   { debug("Enter PCL mode (w/o previous HPGL mode)"); }

<PCL>{ESC}           { debug("Unexpected ESC at top level"); }
<PCL>{ESC}E          { debug("Got PCL Ec E (reset printer)");
                       printer->copies = printer->init_copies;
                       printer->duplex = printer->init_duplex; }
<PCL>{ESC}{TC}       { /* ignore PCL two-char command */
                       debug("{ESC %c}", yytext[1]); }
<PCL>{ESC}{PC}{GC}   { first = yytext[1];
                       group = yytext[2];
                       value = 0; // default
                       BEGIN(PCL_PARAM); }
<PCL_PARAM>{PCLNUM}  { const char *p = yytext;
                       if ((*p == '-') || (*p == '+')) ++p;
                       value = atol(p); /* trunc float */ }
<PCL_PARAM>{zC}      { dopcl(first, group, value, yytext[0]); }
<PCL_PARAM>{ZC}      { dopcl(first, group, value, yytext[0]);
                       BEGIN(PCL); }
<PCL_PARAM>{ESC}     { yyless(0); BEGIN(PCL);
                       debug("Early ESC in param"); }
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

void
joblex(FILE *fp, struct printer *pp, int verbosity)
{
   lineno = 1;
   yyin = fp;
   printer = pp;
   verbose = verbosity;
   yylex();
}

int
yywrap(void)
{
   return 1; // cause scanner to report end-of-file
}

#define PCLCODE(x,y,z) (65536*(x&255) + 256*(y&255) + (z&255))

void dopcl(int first, int group, long value, int param)
{
//   if (verbose) debug("{ESC %c %c %ld %c}", first, group, value, param);

   if (0x60 <= param && param <= 0x7E) param -= 0x20;
   switch (PCLCODE(first,group,param)) {
   case PCLCODE('&','b','W'): // i/o config data
   case PCLCODE('&','p','X'): // transparent data
   case PCLCODE('&','n','W'): // string data
   case PCLCODE('(','f','W'): // symbol set definition
   case PCLCODE(')','s','W'): // font descriptor data
   case PCLCODE('(','s','W'): // character/descriptor data
   case PCLCODE('*','c','W'): // pattepr data
   case PCLCODE('*','b','V'): // raster data (a plane)
   case PCLCODE('*','b','W'): // raster data (a row)
   case PCLCODE('*','v','W'): // CID (Configure Image Data)
   case PCLCODE('*','m','W'): // dither matrix data
   case PCLCODE('*','l','W'): // color lookup table data
   case PCLCODE('*','i','W'): // viewing illuminant data
      debug("Got PCL %d%d3%c, skipping %ld bytes",
            first&255, group&255, param&255, value);
      skip(value);
      break;
   case PCLCODE('&','l','X'):
      printer->copies = value; // affects current and subsequent pages
      debug("Got PCL &l%ldX, set copies = %d", value, printer->copies);
      break;
   case PCLCODE('&','l','S'):
      switch (value) {
      case 0: // simplex
      case 1: // duplex, long edge binding
      case 2: // duplex, short edge binding
         printer->duplex = value > 0;
         debug("Got PCL &l%ldS, set duplex := %d", value, printer->duplex);
         break;
      default: // all other values are to be ignored
         break;
      }
      break;
   default: // ignore
      break;
   }
}

void
skip(long nbytes)
{
#if 1
   while (nbytes-- > 0) {
      int c = input();
      if (c == EOF) break;
   }
#else
   if (fseek(yyin, nbytes, SEEK_CUR) == 0) yyrestart(yyin);
   else exit(111);
#endif
}

void
pjlvar(const char *text)
{
   // Format of text: "name{WS}*={WS}*value{WS*}\r?"
   // Look for these PJL environment variables:
   // COPIES      1..999   num of uncollated copies (each page)
   // DUPLEX      ON|OFF   default duplex mode (job may override)
   // PAPER       A4|LETTER|CUSTOM|etc.  physical media size
   // PLANESINUSE 1|3      1=monochrome, 3=color
   // QTY         1..999   num of collated copies (entire job)

   char *p;
   const char *name, *value;
   int namelen, valuelen;

   // Trim trailing white space from text (\r causes ugly debug output):
   p = (char *) text + strlen(text) - 1;
   while (p > text && isspace(*p)) *p-- = '\0';

   if (p = strchr(text, '='))
   {
      value = p+1; // skip the "="
      name = text;

      // Trim leading and trailing space on name:
      while (isspace(*name)) ++name;
      for (--p; isspace(*p) && p > name; --p);
      namelen = p - name + 1;

      // Trim leading space on value (trailing space removed above):
      while (isspace(*value)) ++value;
      valuelen = strlen(value);

      if (strncasecmp(name, "PLANESINUSE", namelen) == 0) {
         int i = parse_int(value, valuelen);
         joblex_printer_set_color(printer, i > 1);
         debug("Got @PJL SET %s, set color := %d", text, printer->color);
      }
      else if (strncasecmp(name, "DUPLEX", namelen) == 0) {
         int duplex = parse_duplex(value, valuelen);
         joblex_printer_init_duplex(printer, duplex);
         debug("Got @PJL SET %s, init duplex := %d", text, printer->duplex);
      }
      else if (strncasecmp(name, "COPIES", namelen) == 0) {
         int copies = parse_int(value, valuelen);
         joblex_printer_init_copies(printer, copies);
         debug("Got @PJL SET %s, init copies := %d", text, printer->copies);
      }
      else if (strncasecmp(name, "QTY", namelen) == 0) {
         int copies = parse_int(value, valuelen);
         joblex_printer_init_copies(printer, copies);
         debug("Got @PJL SET %s, init copies := %d", text, printer->copies);
      }
      else if (strncasecmp(name, "PAPER", namelen) == 0) {
         joblex_printer_init_paper(printer, strdup(value));
         debug("Got @PJL SET %s, init paper := %s", text, printer->paper);
      }
      else if (strncasecmp(name, "LAYOUT", namelen) == 0) {
         // TODO set duplex and evtl paperSize
         debug("Got @PJL SET %s, ignore for now", text, value);
      }
   }
   // else: not a valid assignment, ignore
}

static int
parse_int(const char *s, int len)
{
   char buf[128];
   strncpy(buf, s, len);
   return atoi(buf);
}

static int // 0 = false, 1 = true
parse_bool(const char *s, int len)
{
   if (strncasecmp(s, "ON", len) == 0) return 1;
   if (strncasecmp(s, "OFF", len) == 0) return 0;
   if (strncasecmp(s, "YES", len) == 0) return 1;
   if (strncasecmp(s, "NO", len) == 0) return 0;
   if (strncasecmp(s, "TRUE", len) == 0) return 1;
   if (strncasecmp(s, "FALSE", len) == 0) return 0;

   return 0; // default
}

static int // 0 = simplex, 1 = duplex
parse_duplex(const char *s, int len)
{
   return parse_bool(s, len);
}

// Input for the PCLXL and PCL5 parsers:
// On each invocation, return next input byte, or EOF on end-of-file

int
pclxl_input()
{
   int c = input();
   if (c == '\n') ++lineno;
   return c;
}

int
pcl5_input()
{
   int c = input();
   if (c == '\n') ++lineno;
   return c;
}

void
joblex_printer_init(struct printer *pp)
{
   pp->can_color = 0;
   pp->can_duplex = 0;
   pp->copies = pp->init_copies = 1;
   pp->duplex = pp->init_duplex = 0;
   pp->paper = pp->init_paper = 0;   // unknown
   pp->pages = 0;
   pp->sheets = 0;
   pp->color = -1;    // unknown
   pp->layout = "?";  // unknown
   memset(pp->structure, '\0', sizeof(pp->structure));
}

void
joblex_printer_can_color(struct printer *pp, int can_color)
{
   pp->can_color = can_color;
   if (!can_color) {
      pp->color = 0;
   }
}

void
joblex_printer_can_duplex(struct printer *pp, int can_duplex)
{
   pp->can_duplex = can_duplex;
   if (!can_duplex) {
      pp->duplex = pp->init_duplex = 0;
   }
}

void
joblex_printer_reset(struct printer *pp)
{
   pp->copies = pp->init_copies;
   pp->duplex = pp->init_duplex;
   pp->paper = pp->init_paper;
}

void
joblex_printer_endpage(struct printer *pp, int duplex, int copies)
{
   // PCL XL: BeginPage/EndPage duplex&copies override PJL settings;
   // here the duplex and copies parameters are negative if missing.
   if (duplex < 0) duplex = pp->duplex;
   if (copies < 0) copies = pp->copies;

   pp->pages += copies;
   pp->sheets += (duplex ? (float) copies/2 : copies);
}

void
joblex_printer_set_color(struct printer *pp, int color)
{
   if (pp->can_color) {
     pp->color = color;
   }
}

void
joblex_printer_init_duplex(struct printer *pp, int duplex)
{
   if (pp->can_duplex) {
      pp->duplex = pp->init_duplex = duplex > 0;
   }
}

void
joblex_printer_set_duplex(struct printer *pp, int duplex)
{
   if (pp->can_duplex) {
      pp->duplex = duplex > 0;
   }
}

void
joblex_printer_init_copies(struct printer *pp, int copies)
{
  pp->copies = pp->init_copies = MAX(0, copies);
}

void
joblex_printer_set_copies(struct printer *pp, int copies)
{
   pp->copies = MAX(0, copies);
}

void
joblex_printer_init_paper(struct printer *pp, const char *name)
{
   pp->paper = pp->init_paper = (char *) name;
}

void
joblex_printer_set_paper(struct printer *pp, const char *name)
{
   pp->paper = (char *) name;
   // TODO Want some extra logic if name != init_paper; if no init_paper, set it
}

void
joblex_printer_set_lang(struct printer *pp, char lang)
{
   int len = strlen(pp->structure);
   if (len+1 < sizeof(pp->structure)) {
      pp->structure[len] = lang;
   }
   else {
      pp->structure[sizeof(pp->structure)-1] = '!';
   }
}
