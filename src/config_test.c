/* Unit testing config */

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "config.h"

const char *me;
int error;

struct pair {
   const char *name;
   const char *value;
};

struct pair white_space_table[] = {
   {"foo", "bar"},
   {"foo.bar", "baz"},
   {"foo.one", "two"},
   {"foo.bar.baz", "quux"},
   {"foo.bar.no", "blanks"},
   {"foo.Bar.very", "tight"},
   {"foo.Bar.file", "ends with incomplete line"},
   {NULL, NULL}
};

struct pair comment_table[] = {
   {"foo", "bar"},
   {"baz", NULL},
   {"final", "test"},
   {NULL, NULL}
};

struct pair value_table[] = {
   {"a", "foo"},
   {"b", "bar baz"},
   {"c", ""},
   {"d", ""},
   {"e", "#\b\"\n;"},
   {"f", NULL},
   {NULL, NULL}
};

void
config_error(const char *fmt, ...)
{
   extern int error;

   va_list ap;
   va_start(ap, fmt);

   vprintf(fmt, ap);
   printf("\n");

   va_end(ap);

   error = 1;
}

static int
foobar_handler(const char *name, const char *value, void *data)
{
   fprintf(stdout, "%s=%s$\n", name, value);

   assert(0 == strcmp(name, "foo"));
   assert(0 == strcmp(value, "bar"));

   return 0;
}

static int
table_handler(const char *name, const char *value, void *data)
{
   struct pair *ptr = (*((struct pair **) data))++;

   fprintf(stdout, "Actual: %s=%s$\n", name, value);

   assert(ptr->name);
   // ptr->value may be null!

   fprintf(stdout, "Expect: %s=%s$\n", ptr->name, ptr->value);

   assert(0 == strcmp(name, ptr->name));
   if (ptr->value)
      assert(0 == strcmp(value, ptr->value));
   else assert(value == NULL);

   return 0;
}

int
main(int argc, char **argv)
{
   FILE *cfp;
   struct pair *pp;
   me = argv[0];

   printf("** Test with UTF-8 BOM:\n");
   cfp = tmpfile();
   fprintf(cfp, "\xef\xbb\xbf");
   fprintf(cfp, "foo=bar");
   rewind(cfp);
   error = 0;
   config_parse_stream(cfp, foobar_handler, NULL);
   fclose(cfp);
   assert(error == 0);

   printf("** Test without the BOM:\n");
   cfp = tmpfile();
   fprintf(cfp, "foo=bar");
   rewind(cfp);
   error = 0;
   config_parse_stream(cfp, foobar_handler, NULL);
   fclose(cfp);
   assert(error == 0);

   printf("** Test with partial BOM (must FAIL):\n");
   cfp = tmpfile();
   fprintf(cfp, "\xef\xbb\xbe");
   fprintf(cfp, "foo=bar");
   rewind(cfp);
   error = 0;
   config_parse_stream(cfp, foobar_handler, NULL);
   fclose(cfp);
   assert(error != 0);

   printf("** Test insignificant white space removal:\n");
   cfp = tmpfile();
   fprintf(cfp, "\n\n   foo   =   bar   \n");
   fprintf(cfp, "  [  foo  ]  bar = baz \n one=two\n");
   fprintf(cfp, " [foo  \"bar\"  ]  baz  =quux \n");
   fprintf(cfp, "no=blanks\n");
   fprintf(cfp, "[foo \"Bar\"]very=tight\n");
   fprintf(cfp, "file = ends with incomplete line"); // no \n at eof
   rewind(cfp);
   error = 0;
   pp = white_space_table;
   config_parse_stream(cfp, table_handler, &pp);
   fclose(cfp);
   assert(error == 0);

   printf("** Test comments:\n");
   cfp = tmpfile();
   fprintf(cfp, "# This is a comment line\n");
   fprintf(cfp, "; and so is this line\n");
   fprintf(cfp, "foo = bar # a setting\n");
   fprintf(cfp, "baz # a flag (true)\n");
   fprintf(cfp, "final=test ; comment"); // no \n at eof
   rewind(cfp);
   error = 0;
   pp = comment_table;
   config_parse_stream(cfp, table_handler, &pp);
   fclose(cfp);
   assert(error == 0);

   printf("** Test weird values:\n");
   cfp = tmpfile();
   fprintf(cfp, "a = foo\n");
   fprintf(cfp, "b = bar baz\n");
   fprintf(cfp, "c = \"\"\n");
   fprintf(cfp, "d = \"\";empty\n");
   fprintf(cfp, "e = \"#\\b\\\"\\n;\" # escapes in quotes\n");
   fprintf(cfp, "f # just a name is a flag (interpret as true)\n");
   rewind(cfp);
   error = 0;
   pp = value_table;
   config_parse_stream(cfp, table_handler, &pp);
   fclose(cfp);
   assert(error == 0);

   printf("** Test config_match_sect():\n");
   assert(8 == config_match_sect("foo.Bar.baz", "foo", "Bar"));
   assert(8 == config_match_sect("foo.Bar.baz", "Foo", "Bar"));
   assert(0 == config_match_sect("foo.Bar.baz", "foo", "bar"));
   assert(4 == config_match_sect("foo.Bar", "foo", NULL));
   assert(4 == config_match_sect("foo.Bar", "foo", ""));
   assert(0 == config_match_sect("foo.Bar.baz", "foo", NULL));
   assert(0 == config_match_sect("foo.Bar.baz", "foo", ""));

   printf("SUCCESS testing config\n");

   return 0;
}
