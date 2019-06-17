#ifndef _printer_h_
#define _printer_h_

// Emulate some of a typical HP Print Environment,
// just about enough for simplistic page counting.

// Flags:    -1=unknown, 0=false, 1=true
// Counts:   -1=unknown, 0, 1, 2, etc.
// Pagesize: <0=unknown, area in mm^2

struct printer
{
   const char *name;

   int copies;                 // current state (count)
   int init_copies;            // initial state (count)
   int default_copies;         // default state (count)
   int can_copies;             // capability (flag)

   int duplex;                 // current state (flag)
   int init_duplex;            // initial state (flag)
   int default_duplex;         // default state (flag)
   int can_duplex;             // capability (flag)

   float pagesize;             // current state (pagesize)
   float init_pagesize;        // initial state (pagesize)
   float default_pagesize;     // default state (pagesize)
   //const char *report_paper;   // TODO unsure

   int init_color;             // initial state (flag)
   int can_color;              // capability (flag)

   int pages;                  // total number of (virtual) pages
   int nsimplex;               // number of simplex pages
   int nduplex;                // number of duplex pages
   double areasum;             // (used to compute avg paper size)
   float sheets;               // sheets printed TODO give up?

   char structure[32];
};

//typedef struct printer printer_t;

extern void printer_init(struct printer *pp, const char *name);
extern void printer_reset(struct printer *pp);
extern void printer_page(struct printer *pp, int duplex, int copies);

extern void printer_set_copies(struct printer *pp, int count);
extern void printer_init_copies(struct printer *pp, int count);
extern void printer_default_copies(struct printer *pp, int count);
extern void printer_can_copies(struct printer *pp, int flag);

extern void printer_set_duplex(struct printer *pp, int flag);
extern void printer_init_duplex(struct printer *pp, int flag);
extern void printer_default_duplex(struct printer *pp, int flag);
extern void printer_can_duplex(struct printer *pp, int flag);

extern void printer_set_pagesize(struct printer *pp, float area);
extern void printer_init_pagesize(struct printer *pp, float area);
extern void printer_default_pagesize(struct printer *pp, float area);

extern void printer_init_color(struct printer *pp, int flag);
extern void printer_can_color(struct printer *pp, int flag);

extern void printer_set_lang(struct printer *pp, char lang);

extern void printer_report(struct printer *pp,
   int *pages, int *duplex, int *color, const char **paper);

#endif // _printer_h_
