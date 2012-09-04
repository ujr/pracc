/* Parser for PCL 5, (c) 2012 Urs-Jakob Ruetschi */

/*
 Two types of Escape Sequences:

 1. Two-Character Escape Sequences: <ESC>X  (X = 48..126 = '0'..'~')
 2. Parametrized Escape Sequences: <ESC>X y z1 # z2 # z3 ... # Zn[data]
    X is the parametrized character (X = 33..47 = '!' .. '/')
    y is the group character (y = 96..126)
    # is a numeric value /[+-]?[0-9]+(\.[0-9]+)?
    zi is the parameter character (zi = 96..126), belongs to previous value
    Zn is the termination char (Zn = 64..94), belongs to previous value
    [data] binary data (eight-bit bytes), eg graphics, fonts, etc.

 Example 1: Ec & l 1 O

    This escape sequence performs a single function.

 Example 2: Ec & l 1 o 2 A

    This escape sequence performs two functions.
    It is the combination of  Ec & l 1 O  and  Ec & l 2 A.
    Note that O turned to o in the combination.

 Rules to combine/shorten printer commands:
 1. First two characters must be the same in all commands combined.
 2. Last char is upper case to mark the end of the combined command.
    All other chars are lower case (cf Example 2).
 3. The commands are performed from left to right.

 The Print Environment

 Four print environments (collection of all settings):
 1. Factory Default Environment
 2. User Default Environment (control panel)
 3. Modified Print Environment (print job)
 4. Overlay Environment (=> Macros)

 PJL "SET" commands override the User Def Env for the duration of the job.
*/

// TC: 0x30..0x7E   two-character escape sequences, eg Ec E
// PC: 0x21..0x2F   parametrized escape sequences
// GC: 0x60..0x7E   group character
// zC: 0x60..0x7E   parameter character
// ZC: 0x40..0x5E   terminating parameter character
// NUM: [-+]?([0-9]{0,5}\.)?[0-9]{1,5}, eg, -12345.00001

// Check https://github.com/sigram/pcl-parser

#include <assert.h>
#include <ctype.h>
#include <math.h>
#include <stdarg.h>
#include <stdio.h>

#include "pcl5.h"
#include "joblex.h"

#define BUG_INVALID_STATE 0

#define PCL5_INPUT pcl5_input // return next byte or EOF on end-of-file
#define PCLPARM(x,y,z) (65536*((x)&255) + 256*((y)&255) + ((z)&255))

enum
pcl5_parse_state {
  INITIAL = 0, ESCAPE = 1, PARAMETRIC = 2, GROUP = 3
};

const char *
pcl5_get_state_name(enum pcl5_parse_state state)
{
   switch (state) {
      case INITIAL:    return "INITIAL";
      case ESCAPE:     return "ESCAPE";
      case PARAMETRIC: return "PARAMETRIC";
      case GROUP:      return "GROUP";
      default:         return "UNKNOWN";
   }
}

static void
pcl5_skip_bytes(int nbytes)
{
   while (nbytes-- > 0) {
      int c = PCL5_INPUT();
      if (c == EOF) break;
   }
}

static void
pcl5_do_char(struct printer *prt, int c)
{
   int copies, duplex;

   switch (c) {
   case 0x0C: // Form Feed
      debug("Got PCL FF (Form Feed)");
      joblex_printer_eject(prt);
      //copies = prt->copies;
      //duplex = prt->duplex;
      //prt->pages += copies;
      //prt->sheets += (duplex ? (float) copies/2 : copies);
      break;
   default: // ignore
      break;
   }
}

static void
pcl5_do_cmd1(struct printer *prt, int c)
{
   switch (c) {
   case 'E': // Printer Reset Command
      debug("Got PCL Ec E (Printer Reset)");
      joblex_printer_reset(prt);
      //prt->copies = prt->init_copies;
      //prt->duplex = prt->init_duplex;
      break;
   default: // ignore
      break;
   }
}

static void
pcl5_do_cmd2(struct printer *prt, int first, int group, int param, double value)
{
   long number = (long) value;

   switch (PCLPARM(first, group, param)) {
   case PCLPARM('&','b','W'): // i/o config data
   case PCLPARM('&','p','X'): // transparent data
   case PCLPARM('&','n','W'): // string data
   case PCLPARM('(','f','W'): // symbol set definition
   case PCLPARM(')','s','W'): // font descriptor data
   case PCLPARM('(','s','W'): // character/descriptor data
   case PCLPARM('*','c','W'): // pattern data
   case PCLPARM('*','b','V'): // raster data (a plane)
   case PCLPARM('*','b','W'): // raster data (a row)
   case PCLPARM('*','v','W'): // CID (Configure Image Data)
   case PCLPARM('*','m','W'): // dither matrix data
   case PCLPARM('*','l','W'): // color lookup table data
   case PCLPARM('*','i','W'): // viewing illuminant data
      debug("Got PCL Ec %c%c#%c, skipping %ld bytes",
            first&255, group&255, param&255, number);
      pcl5_skip_bytes(number);
      break;
   case PCLPARM('&','l','A'):
      debug("Got PCL Ec &l%ldA (Paper Size), ignore for now", number);
      debug("(2=Letter, 26=A4, 27=A3, 101=custom; see HP docs)");
      //TODO joblex_printer_set_pagesize(prt, pcl5_get_paper_size(number));
      break;
   case PCLPARM('&','l','S'):
      switch (number) {
      case 0: // simplex
      case 1: // duplex, long edge binding
      case 2: // duplex, short edge binding
         debug("Got PCL Ec &l%ldS (Simplex/Duplex), set duplex := %d",
               number, number);
         //prt->duplex = number > 0;
         joblex_printer_set_duplex(prt, number);
         break;
      }
      break;
   case PCLPARM('&','l','X'):
      // Affects current and subsequent pages
      debug("Got PCL Ec &l%ldX (Number of Copies), set copies := %d",
            number, (int) number);
      //prt->copies = (int) number;
      joblex_printer_set_copies(prt, (int) number);
      break;
   default: // ignore
      break;
   }
}

static int
pcl5_read_number(int c, double *number)
{
   // PCL numbers: [-+]?([0-9]{0,5}\.)?[0-9]{1,5}

   double value;
   int sign;

   if (c == '-' || c == '+') {
      sign = (c == '-') ? -1 : 1;
      c = PCL5_INPUT();
   }
   else sign = 1;

   value = 0;
   while (isdigit(c)) {
      value *= 10;
      value += c - '0';
      c = PCL5_INPUT();
   }

   if (c == '.') {
      double numer = 0;
      double denom = 1;
      c = PCL5_INPUT();
      while (isdigit(c)) {
         numer *= 10;
         numer += c - '0';
         denom *= 10;
         c = PCL5_INPUT();
      }
      
      value += numer / denom;
   }

   *number = sign * value;

   return c; // the look-ahead
}

static void
pcl5_unexpected(struct printer *prt, int c, int state)
{
   const char *stateName = pcl5_get_state_name(state);
   debug("Got unexpected '%c' (%d) in PCL state %s", c, c, stateName);
}

int
pcl5_parse(struct printer *prt, FILE *logfp, int verbose)
{
   // XXX logfp & verbose presently not used
   enum pcl5_parse_state state;
   int c, cc, first, group, quit, hpgl;
   double value;

   state = INITIAL;
   quit = hpgl = 0;

   while (!quit && (c = PCL5_INPUT()) != EOF)
   {
      switch (state) {
         case INITIAL:
            if (c == 0x1B) state = ESCAPE;
            else if (!hpgl) pcl5_do_char(prt, c);
            break;
         case ESCAPE:
            if (0x21 <= c && c <= 0x2F) { // parametric escape sequence
               first = c;
               state = PARAMETRIC;
               break;
            }
            if (0x30 <= c && c <= 0x7E) { // two-character escape sequence
               pcl5_do_cmd1(prt, c);
               state = INITIAL;
               break;
            }
            pcl5_unexpected(prt, c, state);
            state = INITIAL;
            break;
         case PARAMETRIC:
            if (0x60 <= c && c <= 0x7E) {
               group = c;
               state = GROUP;
               break;
            }
            if (first == '%' && (cc = pcl5_read_number(c, &value)) != c) {
               long number = (long) value;
               if (cc == 'B') {
                  debug("Got PCL Ec %%%ldB, enter HPGL", number);
                  hpgl = 1;
               }
               if (cc == 'A') {
                  debug("Got PCL Ec %%%ldA, leave HPGL", number);
                  hpgl = 0;
               }
               if (cc == 'X' && number == -12345) {
                  debug("Got PCL Ec %%-12345X (UEL)");
                  quit = 1;
               }
               state = INITIAL;
               break;
            }
            pcl5_unexpected(prt, c, state);
            state = INITIAL;
            break;
         case GROUP:
            cc = pcl5_read_number(c, &value);
            if (0x40 <= cc && cc <= 0x5E) {
               pcl5_do_cmd2(prt, first, group, cc, value);
               state = INITIAL; // cc ends cmd group
               break;
            }
            if (0x60 <= cc && cc <= 0x7E) {
               cc -= 0x20;
               pcl5_do_cmd2(prt, first, group, cc, value);
               state = GROUP; // cmd group continues
               break;
            }
            pcl5_unexpected(prt, cc, state);
            state = INITIAL;
            break;
         default:
            assert(BUG_INVALID_STATE);
            break;
      }
   }

   return quit < 0 ? quit : 0;
}
