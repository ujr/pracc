#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>

#include "config.h"
#include "scan.h"
#include "strbuf.h"

#define MAXNAME (256)

struct configfile {
   FILE *fp;
   const char *fn;
   int lineno, eof;
   struct strbuf name; // qualified
   struct strbuf value;
};

static int
streqi(const char *s, const char *t)
{
   while (*s && *t) {
      char cs = tolower(*s++);
      char ct = tolower(*t++);
      if (cs != ct) return 0;
   }
   return !*s && !*t;
}

static int
input(struct configfile *cfp)
{
   int c = fgetc(cfp->fp);
   // Map CR LF (Windows) and plain CR (Mac) to LF:
   if (c == '\r') {
      c = fgetc(cfp->fp);
      if (c != '\n') {
        ungetc(c, cfp->fp);
        c = '\n'; // CR to LF
      }
   }
   if (c == '\n') {
      cfp->lineno += 1;
   }
   if (c == EOF) {
      cfp->eof = 1;
      c = '\n'; // pretend trailing LF
   }
   return c;
}

static void
unput(struct configfile *cfp, int c)
{
   if (c == '\n') cfp->lineno--;
   assert(c == ungetc(c, cfp->fp));
}

static int
skip_utf8_bom(FILE *fp)
{
   // Skip the optional UTF-8 Byte Order Mark (BOM):
   static const char *utf8bom = "\xef\xbb\xbf";
   const char *bomptr;
   int c;

   for (bomptr = utf8bom; *bomptr; bomptr++) {
      c = fgetc(fp);
      if ((char) c != *bomptr) break;
   }

   if (bomptr == utf8bom) {
      ungetc(c, fp);
      return 0; // no BOM
   }
   if (!*bomptr) {
      return 1; // skipped BOM
   }

   return -1; // partial BOM (this is an error)
}

static inline int
iskeychar(int c)
{
   return isalnum(c); // allow '_' and/or '-' and/or '.'?
}

static int // Parse: [section "sub"]
config_parse_section(struct configfile *cfp)
{
   struct strbuf *sp = &cfp->name;
   int c = input(cfp);

   // Skip leading white space:
   while (isspace(c) && c != '\n') c = input(cfp);

   for (;;) {
      if (cfp->eof) return -1;
      if (c == ']') break;
      if (isspace(c)) break;
      if (!iskeychar(c)) return -1;
      strbuf_addc(sp, tolower(c));
      c = input(cfp);
   }

   // Skip inner/trailing white space:
   while (isspace(c) && c != '\n') c = input(cfp);

   if (c == '"') {
      strbuf_addc(sp, '.');
      for (;;) {
         c = input(cfp);
         if (c == '\n') return -1; // incomplete line
         if (c == '"') break; // closing quote
         if (c == '\\') {
            c = input(cfp);
            if (c == '\n') return -1; // incomplete line
         }
         strbuf_addc(sp, c); // don't lowercase
      }
      c = input(cfp);

      // Skip trailing white space:
      while (isspace(c) && c != '\n') c = input(cfp);
   }

   return (c == ']') ? 0 : -1;
}

static char *
config_parse_value(struct configfile *cfp)
{
   int comment = 0, quote = 0, space = 0;

   strbuf_clear(&(cfp->value));

   for (;;) {
      int c = input(cfp);
      if (c == '\n') {
         if (quote) return NULL; // string not terminated
         return cfp->value.buf;
      }
      if (comment) continue;
      if (isspace(c) && !quote) {
         if (strbuf_length(&(cfp->value)) > 0) space += 1;
         continue;
      }
      if (!quote) {
         if (c == '#' || c == ';') {
            comment = 1;
            continue;
         }
      }
      for (; space; space--)
         strbuf_addc(&(cfp->value), ' ');
      if (c == '\\') {
         c = input(cfp);
         switch (c) {
         case '\n': continue;
         case 'n': c = '\n'; break;
         case 't': c = '\t'; break;
         case 'v': c = '\v'; break;
         case 'b': c = '\b'; break;
         case 'r': c = '\r'; break;
         case 'f': c = '\f'; break;
         case 'a': c = '\a'; break;
         case '0': c = '\0'; break;
         case '\\': case '"': break; // themselves
         default: return NULL; // error: bad escape
         }
         strbuf_addc(&(cfp->value), c);
         continue;
      }
      if (c == '"') {
         quote = !quote; // toggle
         continue;
      }
      strbuf_addc(&(cfp->value), c);
   }
}

static int // Parse: name\s*=\s*value\n (or just name\s*)
config_parse_setting(struct configfile *cfp, config_cb_t cb, void *data)
{
   char *name, *value;
   int c;

   for (;;) {
      c = input(cfp);
      if (cfp->eof) break;
      if (!iskeychar(c)) break;
      strbuf_addc(&cfp->name, tolower(c));
   }

   name = cfp->name.buf;
   value = NULL;

   while (isspace(c) && c != '\n') c = input(cfp);

   if (c == '#' || c == ';') {
      unput(cfp, c); // push back
      return cb(name, value, data);
   }

   if (c != '\n') {
      if (c != '=') return -1;
      value = config_parse_value(cfp);
      if (!value) return -1;
   }

   return cb(name, value, data);
}

static int
config_parse(struct configfile *cfp, config_cb_t cb, void *data)
{
   int comment = 0;
   int sectlen = 0;

   if (skip_utf8_bom(cfp->fp) < 0) {
      config_error("bad config: partial UTF-8 BOM");
      return -1;
   }

   for (;;) {
      int c = input(cfp);

      if (c == '\n') {
         if (cfp->eof) return 0;
         comment = 0;
         continue;
      }
      if (comment || isspace(c)) {
         // skip comment or insignificant white space
         continue;
      }
      if (c == '#' || c == ';') {
         comment = 1;
         continue;
      }
      if (c == '[') {
         strbuf_clear(&cfp->name);
         if (config_parse_section(cfp) < 0)
            break;
         strbuf_addc(&cfp->name, '.'); // separator
         sectlen = strbuf_length(&cfp->name);
         continue;
      }
      if (!isalpha(c)) break;
      strbuf_setlen(&cfp->name, sectlen);
      strbuf_addc(&cfp->name, c);
      if (config_parse_setting(cfp, cb, data) < 0)
         break;
   }

   config_error("bad config: line %d in %s",
                cfp->lineno, cfp->fn && cfp->fn[0] ? cfp->fn : "(stream)");
   return -1;
}

int
config_parse_stream(FILE *fp, config_cb_t cb, void *data)
{
   struct configfile cf;

   assert(fp != NULL);
   assert(cb != NULL);
   // data may be NULL

   cf.fp = fp;
   cf.fn = "";
   cf.eof = 0;
   cf.lineno = 1;

   strbuf_init(&cf.name, 0);
   strbuf_init(&cf.value, 0);

   return config_parse(&cf, cb, data);
}

int
config_parse_file(const char *fn, config_cb_t cb, void *data)
{
   int ret = -1;
   struct configfile cf;

   assert(fn != NULL);
   assert(cb != NULL);
   // data may be NULL

   cf.fp = fopen(fn, "r");

   if (cf.fp)
   {
      cf.fn = fn;
      cf.eof = 0;
      cf.lineno = 1;

      strbuf_init(&cf.name, 0);
      strbuf_init(&cf.value, 0);

      ret = config_parse(&cf, cb, data);

      fclose(cf.fp);
   }

   return ret;
}

int
config_match_sect(const char *key, const char *sect, const char *sub)
{
   // key has the form section.name or section.SubSection.name
   const char *base = key;

   // Ignore case when comparing section:
   while (*key && *sect) {
      char kk = tolower(*key++);
      char ss = tolower(*sect++);
      if (kk != ss) return 0;
   }

   if (*key++ != '.') return 0;

   // SubSection part is optional:
   if (sub && *sub) {
      // Honor case when comparing SubSection:
      for (; *key && *sub; key++, sub++) {
         if (*key != *sub) return 0;
      }
      if (*key++ != '.') return 0;
   }
   else {
      // Key must not have a SubSection:
      const char *p = key;
      while (*p) if (*p++ == '.') return 0;
   }

   return key - base; // #chars matched
}

int
config_match_name(const char *key, const char *name)
{
   while (*key && *name) {
      char kk = tolower(*key++);
      char nn = tolower(*name++);
      if (kk != nn) return 0;
   }

   if (*key || *name) return 0;

   return 1;
}

long
config_get_int(const char *name, const char *value)
{
   long ret;
   //while (isspace(*value)) value++;
   if (value[scani(value, &ret)] == '\0')
      return ret;

   config_error("bad config value for %s: '%s'", name, value);
   abort();
}

int
config_get_bool(const char *name, const char *value)
{
   if (!value) return 1; // no value is true (take the name as a flag)
   if (streqi(value, "true") ||
       streqi(value, "yes") ||
       streqi(value, "on")) return 1;
   if (streqi(value, "false") ||
       streqi(value, "no") ||
       streqi(value, "off")) return 0;

   config_error("bad config value for %s: '%s'", name, value);
   abort();
}
