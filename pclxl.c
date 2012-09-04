/* Parser for PCL XL, (c) 2012 Urs-Jakob Ruetschi */

#include <assert.h>
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define OUT_OF_MEMORY 0
#define INVALID_ARGUMENT 0

#define PCLXL_STACK_SIZE 10
#define PCLXL_INPUT pclxl_input // return next byte or EOF on end-of-file
#define MAX(x,y) ((x)>(y)?(x):(y))

#include "pclxl.h"
#include "joblex.h"

enum pclxl_endian {
   PCLXL_ENDIAN_UNKNOWN = 0,
   PCLXL_ENDIAN_LITTLE = 0xCAFE, // low byte first
   PCLXL_ENDIAN_BIG = 0xFECA     // high byte first
};

union pclxl_number {
   unsigned char ubyte;
   unsigned short uint16;
   signed short sint16;
   unsigned long uint32;
   signed long sint32;
   float real32;
   unsigned char raw[4];
};

struct pclxl_pair {
   union pclxl_number x;
   union pclxl_number y;
};

union pclxl_value {
   union pclxl_number number;
   struct pclxl_pair pair;
   // if needed: add pairs and boxes
};

struct pclxl_attr {
   int type;  // Appendix D: UINT16_XY, SINT16_BOX, UINT32_ARRAY, etc.
   int attrid;  // see Appendix E
   union pclxl_value value;
   // if needed: struct pclxl_array [*]array;
   // structure: { int size, pclxl_number *values }
};

struct pclxl_stack {
   struct pclxl_attr *slots;
   int size; // number of allocated slots
   int top; // -1 <= top < size
};

/// Stack functions:

void
pclxl_stack_init(struct pclxl_stack *sp, int size)
{
   assert(size > 0);
   sp->slots = calloc(size, sizeof(struct pclxl_attr));
   assert(sp->slots != OUT_OF_MEMORY);
   sp->top = -1; // empty
   sp->size = size;
}

void
pclxl_stack_free(struct pclxl_stack *sp)
{
   free(sp->slots);
   sp->slots = 0;
   sp->size = 0;
   sp->top = -1;
}

struct pclxl_attr *
pclxl_stack_top(struct pclxl_stack *sp)
{
   return sp->top >= 0 ? &(sp->slots[sp->top]) : 0;
}

struct pclxl_attr * // null if not found
pclxl_stack_find(struct pclxl_stack *sp, int attrid)
{
   int i;
   for (i = 0; i <= sp->top; i++)
   {
      if (sp->slots[i].attrid == attrid)
      {
         return &sp->slots[i];
      }
   }
   return 0; // not found
}

void
pclxl_stack_push(
   struct pclxl_stack *sp,
   int type, union pclxl_value value, int attrid)
{
   assert(sp->top < sp->size - 1);

   struct pclxl_attr *slot;
   slot = &(sp->slots[++sp->top]);

   slot->type = type;
   slot->value = value;
   slot->attrid = attrid;
}

void
pclxl_stack_pop(struct pclxl_stack *sp)
{
   assert(sp->top >= 0);

   struct pclxl_attr *slot;
   slot = &(sp->slots[sp->top--]);

   // Paranoia: override popped slot with zeros
   memset(slot, 0, sizeof(struct pclxl_attr));
}

void
pclxl_stack_clear(struct pclxl_stack *sp)
{
   while (sp->top >= 0)
   {
      pclxl_stack_pop(sp);
   }
   assert(sp->top == -1);
}

/// Log dump functions:

void
pclxl_write_attr(FILE *fp, struct pclxl_attr *attr)
{
   const char *s;
   // [ubyte] eInch Measure
   // [uint16_xy] 600 600 UnitsPerMeasure

   union pclxl_value value = attr->value;

   //fprintf(fp, " %s", pclxl_tag_name(attr->type), fp);

   switch (attr->type) {

   case PCLXL_TAG_UBYTE:
      if (s = pclxl_enum_name(attr->attrid, value.number.ubyte))
         fprintf(fp, " %s", s);
      else
         fprintf(fp, " %d", value.number.ubyte);
      break;
   case PCLXL_TAG_UINT16:
      fprintf(fp, " %d", value.number.uint16);
      break;
   case PCLXL_TAG_SINT16:
      fprintf(fp, " %d", value.number.sint16);
      break;
   case PCLXL_TAG_UINT32:
      fprintf(fp, " %ld", value.number.uint32);
      break;
   case PCLXL_TAG_SINT32:
      fprintf(fp, " %ld", value.number.sint32);
      break;
   case PCLXL_TAG_REAL32:
      fprintf(fp, " %.4f", value.number.real32);
      break;

   case PCLXL_TAG_UBYTE_XY:
      fprintf(fp, " %d %d", value.pair.x.ubyte, value.pair.y.ubyte);
      break;
   case PCLXL_TAG_UINT16_XY:
      fprintf(fp, " %d %d", value.pair.x.uint16, value.pair.y.uint16);
      break;
   case PCLXL_TAG_SINT16_XY:
      fprintf(fp, " %d %d", value.pair.x.sint16, value.pair.y.sint16);
      break;
   case PCLXL_TAG_UINT32_XY:
      fprintf(fp, " %ld %ld", value.pair.x.uint32, value.pair.y.uint32);
      break;
   case PCLXL_TAG_SINT32_XY:
      fprintf(fp, " %ld %ld", value.pair.x.sint32, value.pair.y.sint32);
      break;
   case PCLXL_TAG_REAL32_XY:
      fprintf(fp, " %.4f %.4f", value.pair.x.real32, value.pair.y.real32);
      break;

   // TODO Etc...?

   default:
      fprintf(fp, " #");
   }

   fprintf(fp, " %s\n", pclxl_attr_name(attr->attrid));
}

void
pclxl_logop(FILE *fp, int op, struct pclxl_stack *sp)
{
   if (op == PCLXL_TAG_DATA_LENGTH || op == PCLXL_TAG_DATA_LENGTH_BYTE)
   {
      fprintf(fp, "[data]\n");
   }
   else
   {
      int i;

      for (i = 0; i <= sp->top; i++)
      {
         struct pclxl_attr *attr = &sp->slots[i];
         pclxl_write_attr(fp, attr);
      }
 
      fprintf(fp, "%s\n", pclxl_tag_name(op));
   }
}

/// Input functions:

union pclxl_value
pclxl_read_number(int type, enum pclxl_endian endian)
{
   int i, nbytes, value;
   union pclxl_value result;

   memset(&result, 0, sizeof(union pclxl_value));

   switch (type) {
   case PCLXL_TAG_UBYTE:
   case PCLXL_TAG_UBYTE_XY:
   case PCLXL_TAG_UBYTE_BOX:
   case PCLXL_TAG_UBYTE_ARRAY:
      nbytes = 1;
      break;
   case PCLXL_TAG_UINT16:
   case PCLXL_TAG_UINT16_XY:
   case PCLXL_TAG_UINT16_BOX:
   case PCLXL_TAG_UINT16_ARRAY:
   case PCLXL_TAG_SINT16:
   case PCLXL_TAG_SINT16_XY:
   case PCLXL_TAG_SINT16_BOX:
   case PCLXL_TAG_SINT16_ARRAY:
      nbytes = 2;
      break;
   case PCLXL_TAG_UINT32:
   case PCLXL_TAG_UINT32_XY:
   case PCLXL_TAG_UINT32_BOX:
   case PCLXL_TAG_UINT32_ARRAY:
   case PCLXL_TAG_SINT32:
   case PCLXL_TAG_SINT32_XY:
   case PCLXL_TAG_SINT32_BOX:
   case PCLXL_TAG_SINT32_ARRAY:
   case PCLXL_TAG_REAL32:
   case PCLXL_TAG_REAL32_XY:
   case PCLXL_TAG_REAL32_BOX:
   case PCLXL_TAG_REAL32_ARRAY:
      nbytes = 4;
      break;
   default:
      assert(INVALID_ARGUMENT);
   }

   switch (endian) {
   case PCLXL_ENDIAN_LITTLE:
      for (i = 0; i < nbytes; i++)
      {
         value = PCLXL_INPUT();
         if (value == EOF)
            fatal("Unexpected end-of-file while reading a number");
         result.number.raw[i] = (unsigned char) value;
      }
      break;
   case PCLXL_ENDIAN_BIG:
      for (i = nbytes - 1; i >= 0; i--)
      {
         value = PCLXL_INPUT();
         if (value == EOF)
            fatal("Unexpected end-of-file while reading a number");
         result.number.raw[i] = (unsigned char) value;
      }
      break;
   default:
      assert(INVALID_ARGUMENT);
   }

   return result;
}

union pclxl_value
pclxl_read_pair(int type, enum pclxl_endian endian)
{
   union pclxl_value result;

   result.pair.x = pclxl_read_number(type, endian).number;
   result.pair.y = pclxl_read_number(type, endian).number;

   return result;
}

void
pclxl_skip_bbox(int type, enum pclxl_endian endian)
{
   (void) pclxl_read_number(type, endian);
   (void) pclxl_read_number(type, endian);
   (void) pclxl_read_number(type, endian);
   (void) pclxl_read_number(type, endian);
}

void
pclxl_skip_array(int type, enum pclxl_endian endian)
{
   // TODO check EOF
   union pclxl_value value;
   unsigned long size;
   int i, sizetype = PCLXL_INPUT();

   switch (sizetype) {
   case PCLXL_TAG_UBYTE:
      value = pclxl_read_number(PCLXL_TAG_UBYTE, endian);
      size = value.number.ubyte;
      break;
   case PCLXL_TAG_UINT16:
      value = pclxl_read_number(PCLXL_TAG_UINT16, endian);
      size = value.number.uint16;
      break;
   case PCLXL_TAG_UINT32:
      value = pclxl_read_number(PCLXL_TAG_UINT32, endian);
      size = value.number.uint32;
      break;
   default:
      fatal("Invalid data type for array size: %d", sizetype);
   }

   for (i = 0; i < size; i++)
   {
      (void) pclxl_read_number(type, endian);
   }
}

void
pclxl_skip_data(int type, enum pclxl_endian endian)
{
   unsigned long i, nbytes;
   union pclxl_value value;

   switch (type) {
   case PCLXL_TAG_DATA_LENGTH_BYTE:
      value = pclxl_read_number(PCLXL_TAG_UBYTE, endian);
      nbytes = value.number.ubyte;
      break;
   case PCLXL_TAG_DATA_LENGTH:
      value = pclxl_read_number(PCLXL_TAG_UINT32, endian);
      nbytes = value.number.uint32;
      break;
   default:
      fatal("Invalid data length tag: %d", type);
   }

   for (i = 0; i < nbytes; i++)
   {
      int value = PCLXL_INPUT();
      if (value == EOF)
         fatal("Unexpected end-of-file while reading embedded data");
   }
}

int
pclxl_read_attrid(enum pclxl_endian endian)
{
   union pclxl_value value;
   int tag = PCLXL_INPUT();

   switch (tag) {
   case PCLXL_TAG_ATTR_UBYTE:
      value = pclxl_read_number(PCLXL_TAG_UBYTE, endian);
      return value.number.ubyte;
   case PCLXL_TAG_ATTR_UINT16:
      value = pclxl_read_number(PCLXL_TAG_UINT16, endian);
      return value.number.uint16;
   }

   return -1; // invalid PCL XL format
}

void
pclxl_skip_until(char stop)
{
   int c;
   do { c = PCLXL_INPUT(); }
   while (c != stop && c != EOF);
}

static union pclxl_value novalue; // initialized by pclxl_parse()

int
pclxl_get_op(struct pclxl_stack *sp, enum pclxl_endian endian)
{
   int tag, attrid;
   union pclxl_value value;

   while (1)
   {
      tag = PCLXL_INPUT();

      switch (tag) {

      // Binding stuff:

      case PCLXL_TAG_BIND_ASCII:
         PCLXL_INPUT(); // skip the blank
         pclxl_skip_until('\n');
         return PCLXL_TAG_BIND_ASCII;

      case PCLXL_TAG_BIND_BIN_BE:
         PCLXL_INPUT(); // skip the blank
         pclxl_skip_until('\n');
         return PCLXL_TAG_BIND_BIN_BE;

      case PCLXL_TAG_BIND_BIN_LE:
         PCLXL_INPUT(); // skip the blank
         pclxl_skip_until('\n');
         return PCLXL_TAG_BIND_BIN_LE;

      // Data Types:

      case PCLXL_TAG_UBYTE:
      case PCLXL_TAG_UINT16:
      case PCLXL_TAG_SINT16:
      case PCLXL_TAG_UINT32:
      case PCLXL_TAG_SINT32:
      case PCLXL_TAG_REAL32:
         value = pclxl_read_number(tag, endian);
         attrid = pclxl_read_attrid(endian);
         pclxl_stack_push(sp, tag, value, attrid);
         break;

      case PCLXL_TAG_UBYTE_XY:
      case PCLXL_TAG_UINT16_XY:
      case PCLXL_TAG_SINT16_XY:
      case PCLXL_TAG_UINT32_XY:
      case PCLXL_TAG_SINT32_XY:
      case PCLXL_TAG_REAL32_XY:
         value = pclxl_read_pair(tag, endian);
         attrid = pclxl_read_attrid(endian);
         pclxl_stack_push(sp, tag, value, attrid);
         break;

      case PCLXL_TAG_UBYTE_BOX:
      case PCLXL_TAG_UINT16_BOX:
      case PCLXL_TAG_SINT16_BOX:
      case PCLXL_TAG_UINT32_BOX:
      case PCLXL_TAG_SINT32_BOX:
      case PCLXL_TAG_REAL32_BOX:
         pclxl_skip_bbox(tag, endian);
         attrid = pclxl_read_attrid(endian);
         pclxl_stack_push(sp, tag, novalue, attrid);
         break;

      case PCLXL_TAG_UBYTE_ARRAY:
      case PCLXL_TAG_UINT16_ARRAY:
      case PCLXL_TAG_SINT16_ARRAY:
      case PCLXL_TAG_UINT32_ARRAY:
      case PCLXL_TAG_SINT32_ARRAY:
      case PCLXL_TAG_REAL32_ARRAY:
         pclxl_skip_array(tag, endian);
         attrid = pclxl_read_attrid(endian);
         pclxl_stack_push(sp, tag, novalue, attrid);
         break;

      // All other tags (ie, Operators, dataLength, dataLengthByte,
      // white space and reserved) and the EOF marker: return as-is.

      default:
         return tag;
      }
   }
}

int
pclxl_update_duplex(struct pclxl_stack *sp, int duplex)
{
   if (pclxl_stack_find(sp, PCLXL_ATTR_DUPLEX_PAGE_MODE))
      return 1;

   if (pclxl_stack_find(sp, PCLXL_ATTR_SIMPLEX_PAGE_MODE))
      return 0;

   return duplex; // keep previous setting
}

int
pclxl_get_page_copies(struct pclxl_stack *sp)
{
   struct pclxl_attr *attr = pclxl_stack_find(sp, PCLXL_ATTR_PAGE_COPIES);
   if (attr != NULL)
   {
      switch (attr->type) {
      case PCLXL_TAG_UBYTE: return attr->value.number.ubyte;
      case PCLXL_TAG_UINT16: return attr->value.number.uint16;
      case PCLXL_TAG_UINT32: return attr->value.number.uint32;
      }
   }

   return 1; // assume 1 copy if PAGE_COPIES is missing
}

int
pclxl_parse(struct printer *prt, FILE *logfp, int verbose)
{
   // logfp: where to log (job dump) info
   // verbose: 0=silent, 1=pages, 2=all ops

   struct pclxl_stack stack;
   enum pclxl_endian endian;
   int op, quit = 0, copies = 1;

   endian = PCLXL_ENDIAN_UNKNOWN;
   memset(&novalue, 0, sizeof(novalue));
   pclxl_stack_init(&stack, PCLXL_STACK_SIZE);

   // TODO Don't modify prt directly, use joblex_printer_foo() instead

   while (!quit)
   {
      pclxl_stack_clear(&stack);

      op = pclxl_get_op(&stack, endian);

      if (verbose > 1)
         pclxl_logop(logfp, op, &stack);

      switch (op) {

      case PCLXL_TAG_BIND_ASCII:
         quit = -1; // TODO Don't know how to read this
         break;
      case PCLXL_TAG_BIND_BIN_LE:
         endian = PCLXL_ENDIAN_LITTLE;
         break;
      case PCLXL_TAG_BIND_BIN_BE:
         endian = PCLXL_ENDIAN_BIG;
         break;

      case PCLXL_TAG_DATA_LENGTH:
      case PCLXL_TAG_DATA_LENGTH_BYTE:
         // ignore data (it belongs to the preceding command)
         pclxl_skip_data(op, endian);
         break;

      case PCLXL_TAG_BEGIN_PAGE:
         if (verbose == 1) pclxl_logop(logfp, op, &stack);
         prt->duplex = pclxl_update_duplex(&stack, prt->duplex);
         // TODO Get paper size; compute an ``average paper size''
         break;
      case PCLXL_TAG_END_PAGE:
         if (verbose == 1) pclxl_logop(logfp, op, &stack);
         copies = MAX(0, pclxl_get_page_copies(&stack));
         // With PCL XL, copies may be 0, meaning to not print this page.
         // TODO How does this interact with PJL's COPIES/QTY?
         // TODO Update prt->copies?
         prt->pages += copies;
         prt->sheets += (prt->duplex ? (float) copies/2 : copies);
         break;

      case PCLXL_TAG_BEGIN_SESSION:
         if (verbose == 1) pclxl_logop(logfp, op, &stack);
         break;

      case PCLXL_TAG_END_SESSION:
         // What if there's another session? Appendix H of the PCL XL
         // Feature Reference says that all PCL XL sessions MUST be
         // invoked through a PJL ENTER LANGUAGE command and that
         // EndSession in the stream SHOULD be followed by a UEL.
         if (verbose == 1) pclxl_logop(logfp, op, &stack);
         quit = 1;
         break;

      case EOF:
         if (verbose > 0) pclxl_logop(logfp, op, &stack);
         quit = 1;
         break;
      }
   }

   pclxl_stack_free(&stack);

   return quit < 0 ? quit : 0;
}
