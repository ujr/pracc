/*
 * Implementation of symbol tables as hash tables,
 * mapping strings to symbol structures.
 *
 * Functions symget() and symput() return NULL
 * if the given name is NULL or not found.
 */
#include "symtab.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

static unsigned long hash(const char *s);
static int grow(struct symtab *st, int increment);

void syminit(struct symtab *st, int nbucks)
{
   assert(st);

   if (nbucks <= 0) nbucks = 1;

   st->size = nbucks;
   st->bucks = (struct symbol **) 0;
   st->symbase = (struct symbol *) 0;
   st->nextsym = (struct symbol *) 0;
   st->beyond = (struct symbol *) 0;
}

void symkill(struct symtab *st)
{
   assert(st);

   if (st->symbase) free(st->symbase);
   if (st->bucks) free(st->bucks);

   st->size = 0;
   st->bucks = 0;
   st->symbase = st->nextsym = st->beyond = 0;
}

struct symbol *symget(struct symtab *st, const char *name)
{
   assert(st);

   if (st->bucks && name) {
      unsigned long h = hash(name) % st->size;
      struct symbol *s = st->bucks[h];
   
      while (s) { // bucket list
         if (strcmp(s->name, name))
            s = s->next;
         else return s; // found
      }
   }
   return (struct symbol *) 0; // not found
}

struct symbol *symput(struct symtab *st, const char *name)
{
   unsigned long h;
   struct symbol *s, *prev;

   assert(st);

   if (!name) return 0;
   s = symget(st, name);
   if (s) return s; // symbol exists

   if (st->nextsym == st->beyond) {
      if (grow(st, st->size))
         return (struct symbol *) 0; // ENOMEM
   }

   h = hash(name) % st->size;
   prev = st->bucks[h];

   s = st->nextsym++;
   s->name = name;
   s->next = prev;
   s->sval = 0;

   st->bucks[h] = s;
//fprintf(stderr, "symput(%s): h=%ld, prev=0x%lx\n", name, h, prev);

   return s; // symbol added
}

static int grow(struct symtab *st, int increment)
{
   int i, oldsize;
   struct symbol *oldsyms;

   if (st->bucks) {
      oldsize = st->size;
      oldsyms = st->symbase;
      free(st->bucks);
   }
   else {
      oldsize = 0;
      oldsyms = 0;
   }

//fprintf(stderr, "grow to %d+%d syms\n", oldsize, increment);
   syminit(st, oldsize + increment);
   st->bucks = calloc(st->size, sizeof(struct symbol *));
   if (!st->bucks) return -1; // ENOMEM
   st->symbase = calloc(st->size, sizeof(struct symbol));
   if (!st->symbase) return -1; // ENOMEM
   st->nextsym = st->symbase;
   st->beyond = st->symbase + st->size;

//fprintf(stderr, " copying %d syms...\n", oldsize);
   for (i = 0; i < oldsize; i++) {
      const char *name = oldsyms[i].name;
      unsigned long h = hash(name) % st->size;
      struct symbol *prev = st->bucks[h];
      struct symbol *s = st->nextsym++;

//fprintf(stderr, "  reput %s...\n", name);
      *s = oldsyms[i];
      s->next = prev;
      st->bucks[h] = s;
   }
   if (oldsyms) free(oldsyms);
//fprintf(stderr, " done: size=%d bucks=0x%lx syms=0x%lx\n",
//        st->size, st->bucks, st->symbase);

   return 0; // OK
}

void symeach(struct symtab *st, void (*func)(struct symbol *sym))
{
   int i, n = 0;

   assert(st);

   if (st->bucks) for (i = 0; i < st->size; i++) {
      struct symbol *sym = st->bucks[i];
      while (sym) {
         if (func) (*func)(sym);
         sym = sym->next; ++n;
      }
   }
}

int symcount(struct symtab *st)
{
   int i, n = 0;

   assert(st);

   if (st->bucks) for (i = 0; i < st->size; i++) {
      struct symbol *s = st->bucks[i];
      while (s) { ++n; s = s->next; }
   }
   return n;
}

void symdump(struct symtab *st, FILE *fp)
{
   int i, n = 0;

   assert(st);

   if (st->bucks) for (i = 0; i < st->size; i++) {
      struct symbol *s = st->bucks[i];
      if (s) {
         if (fp) fprintf(fp, "%d:", i);
         while (s) {
            if (fp) fprintf(fp, " %s", s->name);
            s = s->next; ++n;
         }
         if (fp) fprintf(fp, "\n");
      }
   }
}

/* This is the hash function that Bernstein uses in his CDB.
 * See his cdb_hash.c, which is in the public domain.
 */
static unsigned long hash(const char *s)
{
   unsigned long h = 5381;
   while (*s) {
      h += (h << 5);
      h ^= *s++;
   }
   return h;
}

