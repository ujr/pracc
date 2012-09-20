/* Paper sizes, (c) 2012 Urs-Jakob Ruetschi */

#include <float.h>    // FLT_MAX
#include <math.h>     // fabs()
#include <string.h>   // strcasecmp()

static struct papersize_t {
   const char *name;
   int pcl5key;
   int pcl6key;
   float width;   // in millimeters
   float height;  // in millimeters
} papersizetab[] = {
   "LETTER",     2,  0,  215.9, 279.4,  // eLetterPaper      8.5x11 in
   "LEGAL",      3,  1,  215.9, 355.6,  // eLegalPaper       8.5x14 in
   "LEDGER",     6,  4,  432.0, 279.0,  // eLedgerPaper      11x17 in
   "A6",        -1, 17,  105,   148,    // eA6Paper
   "A5",        -1, 16,  148,   210,    // eA5Paper
   "A4",        26,  2,  210,   297,    // eA4Paper
   "A3",        27,  5,  297,   420,    // eA3Paper
   "EXECUTIVE",  1,  3,  184,   267,    // eExecPaper        7.25x10.5 in
   "JISB4",     -1, 10,  257,   364,    // eJB4Paper
   "JISB5",     -1, 11,  182,   257,    // eJB5Paper
   "JISB6",     -1, 18,  128,   182,    // eJB6Paper

   "COM10",     81,  6,  105,   241,    // eCOM10Envelope    4.125x9.5 in
   "C5",        91,  8,  162,   229,    // eC5Envelope
   "DL",        90,  9,  110,   220,    // eDLEnvelope
   "MONARCH",   80,  7,   98.4, 190.5,  // eMonarchEnvelope  3.875x7.5 in
   "B5",       100, 12,  176,   250,    // eB5Envelope

   "JPOST",     -1, 14,  100,   148,    // eJPostcard
   "JPOSTD",    -1, 15,  200,   148,    // eJDoublePostcard
   0,            0,  0,    0.0,   0.0
};

int
papersize_lookup_name(const char *name, float *width, float *height)
{
   struct papersize_t *psp;
   if (!name) return 0; // no such name
   for (psp = papersizetab; psp->name; psp++)
   {
      if (strcasecmp(name, psp->name) == 0)
      {
         if (width) *width = psp->width;
         if (height) *height = psp->height;
         return 1; // found
      }
   }
   return 0; // no such name
}

int
papersize_lookup_pcl5(int key, char **name, float *width, float *height)
{
   struct papersize_t *psp;
   if (key < 0) return 0; // no such key
   for (psp = papersizetab; psp->name; psp++)
   {
      if (key == psp->pcl5key)
      {
         if (name) *name = (char *) psp->name;
         if (width) *width = psp->width;
         if (height) *height = psp->height;
         return 1; // found
      }
   }
   return 0; // no such key
}

int
papersize_lookup_pcl6(int key, char **name, float *width, float *height)
{
   struct papersize_t *psp;
   if (key < 0) return 0; // no such key
   for (psp = papersizetab; psp->name; psp++)
   {
      if (key == psp->pcl6key)
      {
         if (name) *name = (char *) psp->name;
         if (width) *width = psp->width;
         if (height) *height = psp->height;
         return 1; // found
      }
   }
   return 0; // no such key
}

float
papersize_lookup_area(float area, char **name, float *width, float *height)
{
   struct papersize_t *psp = papersizetab;
   float bestdelta = fabs(area - (psp->width * psp->height));
   struct papersize_t *best = psp;

   for (++psp; psp->name; psp++)
   {
      float delta = fabs(area - (psp->width * psp->height));
      if (delta < bestdelta)
      {
         best = psp;
         bestdelta = delta;
      }
   }

   if (name) *name = (char *) best->name;
   if (width) *width = best->width;
   if (height) *height = best->height;

   return bestdelta;
}

#if 0 // TESTING
#include <stdio.h>
#include <stdlib.h> // atof()

int
main(int argc, char **argv)
{
   if (argc != 2) {
      fprintf(stderr, "Usage: %s name|area\n", argv[0]);
      return 127;
   }

   char *name = argv[1];
   float width, height, area = atof(name);

   if (area > 0) {
      float delta = papersize_lookup_area(area, &name, &width, &height);
      printf("%s (%.1f x %.1f mm), delta = %.1f%%\n",
             name, width, height, 100.0*delta/area);
   }
   else {
      int ok = papersize_lookup_name(name, &width, &height);
      if (ok) printf("%s: %.1f x %.1f mm, area is %.0f\n",
                     name, width, height, width*height);
      else printf("Unknown paper size: %s\n", name);
   }

   return 0;
}
#endif
