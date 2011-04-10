#ifndef _SYMTAB_H_
#define _SYMTAB_H_

#include <stdio.h>

struct symbol {
   const char *name;
   const char *sval;
   long int i0, i1, i2;
   struct symbol *next;
};

struct symtab {
   int size;
   struct symbol **bucks;
   struct symbol *symbase;
   struct symbol *nextsym;
   struct symbol *beyond;
};

void syminit(struct symtab *st, int size);
void symkill(struct symtab *st);

struct symbol *symget(struct symtab *st, const char *name);
struct symbol *symput(struct symtab *st, const char *name);

void symeach(struct symtab *st, void (*func)(struct symbol *sym));
int symcount(struct symtab *st);
void symdump(struct symtab *st, FILE *fp);

#endif // _SYMTAB_H_
