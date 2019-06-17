#ifndef _strbuf_h_
#define _strbuf_h_

#include <stdarg.h>

struct strbuf
{
   char *buf;
   size_t size;
   size_t len;
};

extern void
strbuf_init(struct strbuf *sp, size_t hint);

extern void
strbuf_free(struct strbuf *sp);

extern struct strbuf *
strbuf_addc(struct strbuf *sp, int c);

extern struct strbuf *
strbuf_adds(struct strbuf *sp, const char *s);

extern struct strbuf *
strbuf_addb(struct strbuf *sp, const char *buf, size_t len);

extern struct strbuf *
strbuf_addf(struct strbuf *sp, const char *fmt, ...);

extern struct strbuf *
strbuf_addfv(struct strbuf *sp, const char *fmt, va_list ap);

extern void
strbuf_setlen(struct strbuf *sp, int len);

extern void
strbuf_clear(struct strbuf *sp);

#define strbuf_length(sp) ((sp)->len)
#define strbuf_last(sp) ((sp)->len > 0 ? (sp)->buf[(sp)->len - 1] : '\0')

#endif // _strbuf_h_
