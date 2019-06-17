// Part of Pracc
// Copyright (c) 2012-2013 by Urs-Jakob Ruetschi
//
// Emulate some of a typical HP Print Environment,
// just about enough for simplistic page counting.
//
// Print Environments:
//  1. Factory Default (burnt into printer) (here: default)
//  2. User Default (set from control panel) (here: initial)
//  3. Modified (set by print job) (here: current)
//  4. Overlay (related to macros) (here: ignored)
//
// Interesting variables in Print Environments:
//  - Number of Copies:  default  initial
//  - Duplex / Simplex:  default  initial
//  - Paper Size:        (tray)   initial
// 
// Printer Reset (PCL EcE): restores User Default Environment.
// Cold Reset (power cycle): restores Factory Default Environment.

#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "papersize.h"
#include "printer.h"

#define MAX(a,b) ((a) > (b) ? (a) : (b))

static int getcopies(struct printer *pp, int count);
static int getduplex(struct printer *pp, int flag);
static int getcolor(struct printer *pp, int flag);
static float getarea(struct printer *pp, float area);

void
printer_init(struct printer *pp, const char *name)
{
   pp->name = name;

   pp->copies = 1; // assume 1 copy
   pp->init_copies = 1;
   pp->default_copies = 1;
   pp->can_copies = 1;

   pp->duplex = 0; // assume simplex
   pp->init_duplex = 0;
   pp->default_duplex = 0;
   pp->can_duplex = 1;

   pp->pagesize = 0; // unknown
   pp->init_pagesize = 0;
   pp->default_pagesize = 0;

   pp->init_color = -1; // unknown
   pp->can_color = 1;

   pp->pages = 0;
   pp->nsimplex = 0;
   pp->nduplex = 0;
   pp->areasum = 0;
   pp->sheets = 0; // TODO drop?

   memset(pp->structure, 0, sizeof(pp->structure));
}

void
printer_reset(struct printer *pp)
{
   // Printer Reset (PCL EcE):
   pp->copies = pp->init_copies;
   pp->duplex = pp->init_duplex;
   pp->pagesize = pp->init_pagesize;
   // no effect on color
}

void
printer_page(struct printer *pp, int duplex, int copies)
{
   // Complete a page: update counters
   float area;

   // PCL XL BeignPage/EndPage's duplex&copies override PJL settings!
   // Here the copies and duplex parameters are -1 if missing.
   copies = getcopies(pp, copies);
   duplex = getduplex(pp, duplex);
   area = getarea(pp, -1);

   // TODO Special logic if duplex and area != last_area

   pp->pages += copies;

   if (duplex) pp->nduplex += copies;
   else pp->nsimplex += copies;

   pp->areasum += area*copies;

   pp->sheets += (duplex ? (float) copies/2 : copies);
}

void
printer_report(
   struct printer *pp,
   int *pages, int *duplex, int *color, const char **paper)
{
   if (pages) *pages = pp->pages;

   // At least one duplex page?
   if (duplex) *duplex = MAX(0, pp->nduplex);

   if (color) *color = getcolor(pp, -1);

   if (paper)
   {
      float width, height;
      float avgarea = pp->pages > 0 ? pp->areasum/pp->pages : 0;
      papersize_lookup_area(avgarea, paper, &width, &height);
   }
}

static int
getcopies(struct printer *pp, int count)
{
   if (count < 0) count = pp->copies;
   if (count < 0) count = pp->init_copies;
   if (count < 0) count = pp->default_copies;
   if (count < 0) count = 1; // pretend

   return pp->can_copies ? count : 1;
}

static int
getduplex(struct printer *pp, int flag)
{
   if (flag < 0) flag = pp->duplex;
   if (flag < 0) flag = pp->init_duplex;
   if (flag < 0) flag = pp->default_duplex;
   if (flag < 0) flag = 0; // pretend

   return pp->can_duplex ? flag : 0;
}

static int
getcolor(struct printer *pp, int flag)
{
   if (flag < 0) flag = pp->init_color;
   //if (flag < 0) ???

   return pp->can_color ? flag : 0;
}

static float
getarea(struct printer *pp, float area)
{
   if (area <= 0) area = pp->pagesize;
   if (area <= 0) area = pp->init_pagesize;
   if (area <= 0) area = pp->default_pagesize;
   //if (area <= 0) ???

   return area;
}



void
printer_set_copies(struct printer *pp, int count)
{
   // count may be 0 for PCL XL ?

   pp->copies = count;
}

void
printer_init_copies(struct printer *pp, int count)
{
   assert(count > 0);

   pp->copies = count;
   pp->init_copies = count;
}

void
printer_default_copies(struct printer *pp, int count)
{
   assert(count > 0);

   pp->copies = count;
   pp->init_copies = count;
   pp->default_copies = count;
}

void
printer_can_copies(struct printer *pp, int flag)
{
   pp->can_copies = flag;

   if (!flag)
   {
      printer_default_copies(pp, 1);
   }
}



void
printer_set_duplex(struct printer *pp, int flag)
{
   pp->duplex = flag; // <0 = unknown, 0 = simplex, >0 = duplex

   if (pp->init_duplex < 0 && flag >= 0)
   {
      pp->init_duplex = flag;
   }
}

void
printer_init_duplex(struct printer *pp, int flag)
{
   pp->duplex = flag;
   pp->init_duplex = flag;
}

void
printer_default_duplex(struct printer *pp, int flag)
{
   pp->duplex = flag;
   pp->init_duplex = flag;
   pp->default_duplex = flag;
}

void
printer_can_duplex(struct printer *pp, int flag)
{
   pp->can_duplex = flag;
}



void
printer_init_color(struct printer *pp, int flag)
{
   pp->init_color = flag;
}

void
printer_can_color(struct printer *pp, int flag)
{
   pp->can_color = flag;
}



void
printer_set_pagesize(struct printer *pp, float area)
{
   pp->pagesize = area;

   if (pp->init_pagesize < 0)
   {
      pp->init_pagesize = area;
   }
}

void
printer_init_pagesize(struct printer *pp, float area)
{
   pp->pagesize = area;
   pp->init_pagesize = area;
}

void
printer_default_pagesize(struct printer *pp, float area)
{
   pp->pagesize = area;
   pp->init_pagesize = area;
   pp->default_pagesize = area;
}



void
printer_set_lang(struct printer *pp, char lang)
{
   int len = strlen(pp->structure);

   if (len+1 < sizeof(pp->structure))
   {
      pp->structure[len] = lang;
      pp->structure[len+1] = 0; // terminate
   }
   else
   {
      pp->structure[sizeof(pp->structure)-2] = '!';
      pp->structure[sizeof(pp->structure)-1] = 0; // terminate
   }
}
