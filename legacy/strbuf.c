#include <stdlib.h>

#include "pracc.h"
#include "strbuf.h"

/* The internal memory allocation routine:
 * Ensure that the given strbuf has a capacity of at least
 * size (rounded up to an integer multiple of 16) bytes
 * and return the new capacity. Return 0/ENOMEM on error.
 */
static
int sbspace(struct strbuf *sb, int size)
{
//  logfmt("sbspace: requested min size: %d\n", size); // XXX
  size += 15;
  size &= ~15;

  if (sb->x == 0) sb->s = 0;
  if (sb->x == 0 || size > sb->a) {
  	char *mem = (char *) realloc(sb->x, size);
  	if (mem == 0) die(111, "sbspace");
//  	logfmt("sbspace: alloc'ed %d bytes\n", size); // XXX

  	sb->x = mem;
  	sb->a = size;
  }

  return size; // new size
}

/*
 * Initialisation, expansion, and trimming routines.
 */

int sbinit(struct strbuf *sb, int size)
{ // initialise sb with a capacity of at least size bytes
  sbfree(sb);
  return sbspace(sb, size); // new size
}

int sbmore(struct strbuf *sb, int size)
{ // make sure we can hold at least size more bytes!
  if (!sb->x) return sbspace(sb, size);
  if (sb->s + size <= sb->a) return sb->a;
  return sbspace(sb, sb->a + size); // new size
}

void sbfree(struct strbuf *sb)
{ // free allocated memory
  if (sb->x) free(sb->x);
  sb->x = (char *) 0;
  sb->s = sb->a = 0;
}

void sbzero(struct strbuf *sb)
{ // trim sb to length zero
  sb->s = 0;
  if (sb->x && (sb->a > 0)) sb->x[0] = '\0';
}

/*
 * Next are routines to append content to a string buffer.
 * They all return the new string length (not buffer capacity)
 * or zero if there was not enough memory.
 */

int sbaddc(struct strbuf *sb, char c)
{ // append char c to strbuf sb
  if (!sbmore(sb, 1)) return 0;
  sb->x[sb->s++] = c;
  return sb->s; // new length
}

int sbadds(struct strbuf *sb, const char *s)
{ // append string s to strbuf sb
  return sbaddb(sb, s, strlen(s));
}

int sbaddb(struct strbuf *sb, const char *buf, int len)
{ // append buf[0..len-1] to strbuf sb
  register char *p;
  if (!sbmore(sb, len)) return 0;
  p = sb->x + sb->s; // init target pointer
  sb->s += len; // record new string length
  while (len--) *p++ = *buf++; // copy over
  return sb->s; // new length
}

int sbadd(struct strbuf *sb, struct strbuf *sb2)
{ // append sb2 to sb
  return sbaddb(sb, sb->x, sb->s);
}

int sbadd0(struct strbuf *sb)
{ // append a C string terminator
  return sbaddc(sb, '\0');
}

/*
 * Convenience routines that convert to C strings.
 */

char *sbnull(struct strbuf *sb)
{ // null-terminate and return C string
  if (sb->x && (sb->s > 0) && (sb->x[sb->s-1] == '\0')) return sb->x;
  return (sbadd0(sb) == 0) ? (char *) 0 : sb->x; // valid C string
}

char *sbcat2(struct strbuf *sb, char *s, char *t)
{ // concat s and t using sb, return C string
  int slen = strlen(s);
  int tlen = strlen(t);
  if (!sbinit(sb, slen+tlen+1)) return (char *) 0;
  (void) sbaddb(sb, s, slen);
  (void) sbaddb(sb, t, tlen);
  return sbadd0(sb) ? sb->x : (char *) 0;
}

char *sbcat3(struct strbuf *sb, char *s, char *t, char *u)
{ // concat s and t and u using sb, return C string
  int slen = strlen(s);
  int tlen = strlen(t);
  int ulen = strlen(u);
  if (!sbinit(sb, slen+tlen+ulen+1)) return (char *) 0;
  (void) sbaddb(sb, s, slen);
  (void) sbaddb(sb, t, tlen);
  (void) sbaddb(sb, u, ulen);
  return sbadd0(sb) ? sb->x : (char *) 0;
}
