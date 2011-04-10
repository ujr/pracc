#ifndef _STRBUF_H_
#define _STRBUF_H_

struct strbuf {
  char *x; // base ptr, null if not alloc
  int a;   // #bytes allocated
  int s;   // #bytes in string (s <= a)
};

int sbinit(struct strbuf *sb, int size);
int sbmore(struct strbuf *sb, int size);
void sbfree(struct strbuf *sb);
void sbzero(struct strbuf *sb);

int sbaddc(struct strbuf *sb, char c);
int sbadds(struct strbuf *sb, const char *s);
int sbaddb(struct strbuf *sb, const char *buf, int len);
int sbadd(struct strbuf *sb, struct strbuf *sb2);
int sbadd0(struct strbuf *sb); // append '\0'

char *sbnull(struct strbuf *sb);
char *sbcat2(struct strbuf *sb, char *s, char *t);
char *sbcat3(struct strbuf *sb, char *s, char *t, char *u);

#endif /* _STRBUF_H_ */
