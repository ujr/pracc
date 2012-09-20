#ifndef _JOBLEX_H_
#define _JOBLEX_H_

#include <stdio.h>

struct printer { // TODO Rename struct joblex_printer
   int copies, init_copies; // default: 1
   int duplex, init_duplex; // -1=unknown, 0=simplex, 1=duplex
   char *paper, *init_paper;// -1=unknown, avg area of media
   int pages;               // pages printed
   float sheets;            // sheets printed
   int color;               // -1=unknown, 0=false, 1=true
   char *layout;            // TODO Nup, duplex, booklet
   char structure[32];
};

extern void joblex(FILE *fp, struct printer *printer, int verbosity);

extern void joblex_printer_init(struct printer *pp);
extern void joblex_printer_reset(struct printer *pp);
extern void joblex_printer_endpage(struct printer *pp, int duplex, int copies);
extern void joblex_printer_set_color(struct printer *pp, int color);
extern void joblex_printer_init_duplex(struct printer *pp, int duplex);
extern void joblex_printer_set_duplex(struct printer *pp, int duplex);
extern void joblex_printer_init_copies(struct printer *pp, int copies);
extern void joblex_printer_set_copies(struct printer *pp, int copies);
extern void joblex_printer_init_paper(struct printer *pp, const char *name);
extern void joblex_printer_set_paper(struct printer *pp, const char *name);
extern void joblex_printer_set_lang(struct printer *pp, char lang);

extern void debug(const char *fmt, ...);
extern void fatal(const char *fmt, ...);
// TODO pass in a line number or byte offset (for better error reporting)

#endif // _JOBLEX_H_
