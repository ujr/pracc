#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "strbuf.h"

#define OUT_OF_MEMORY (0)
#define GROWFUNC(x) (((x)+16)*3/2) // lifted from Git
#define MAX(x,y) ((x) > (y) ? (x) : (y))

static struct strbuf *strbuf_ready(struct strbuf *sb, int bytes);
static struct strbuf *strbuf_grow(struct strbuf *sb, int bytes);

void
strbuf_init(struct strbuf *sp, size_t hint)
{
   sp->buf = NULL;
   sp->size = sp->len = 0;
   strbuf_grow(sp, hint > 0 ? hint : 16);
   sp->buf[0] = '\0'; // terminate
}

void
strbuf_free(struct strbuf *sp)
{
   if (sp->size > 0) {
      free(sp->buf);
   }
   sp->buf = ""; // empty
   sp->size = sp->len = 0;
}

struct strbuf *
strbuf_addc(struct strbuf *sp, int c)
{
   strbuf_ready(sp, 1);

   sp->buf[sp->len++] = (char) (c & 255);
   sp->buf[sp->len] = '\0'; // terminate

   return sp;
}

struct strbuf *
strbuf_adds(struct strbuf *sp, const char *s)
{
   return strbuf_addb(sp, s, strlen((char *) s));
}

struct strbuf *
strbuf_addb(struct strbuf *sp, const char *buf, size_t len)
{
   strbuf_ready(sp, len);

   memcpy(sp->buf + sp->len, buf, len);
   sp->len += len;
   sp->buf[sp->len] = '\0'; // terminate

   return sp;
}

struct strbuf *
strbuf_addf(struct strbuf *sp, const char *fmt, ...)
{
   va_list ap;

   va_start(ap, fmt);
   strbuf_addfv(sp, fmt, ap);
   va_end(ap);

   return sp;
}

struct strbuf *
strbuf_addfv(struct strbuf *sp, const char *fmt, va_list ap)
{
   size_t avail, chars;

   avail = sp->size - sp->len;
   chars = vsnprintf(sp->buf + sp->len, avail, fmt, ap);

   if (chars >= avail) // equal means no space for \0
   {
      strbuf_grow(sp, chars+1);

      avail = sp->size - sp->len;
      chars = vsnprintf(sp->buf + sp->len, avail, fmt, ap);
   }

   sp->len += chars;

   return sp;
}

void
strbuf_setlen(struct strbuf *sp, int len)
{
   assert(len < sp->size);
   sp->len = len;
   sp->buf[len] = '\0';
}

void
strbuf_clear(struct strbuf *sp)
{
   sp->buf[0] = '\0';
   sp->len = 0;
}

static struct strbuf *
strbuf_ready(struct strbuf *sb, int bytes)
{
   int avail = sb->size - sb->len;

   // Note: equal means no space for \0
   if (avail > bytes) return sb;

   return strbuf_grow(sb, bytes);
}

static struct strbuf *
strbuf_grow(struct strbuf *sb, int bytes)
{
   if (bytes < 1) return sb; // do nothing

   int requested = sb->size + bytes;
   int standard = GROWFUNC(sb->size);
   int newsize = MAX(requested, standard);

   //int more = MAX(STRBUF_INCREMENT, bytes+4);
   //int newsize = sb->size + more;

   char *ptr = realloc(sb->buf, newsize);
   assert(ptr != OUT_OF_MEMORY);
//fprintf(stderr, "strbuf_grow: %d to %d bytes\n", sb->size, newsize);

   sb->buf = ptr;
   sb->size = newsize;

   return sb;
}
