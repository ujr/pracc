#ifndef _PS_H_
#define _PS_H_

#define PS_MSG_COOKIE 1
#define PS_MSG_PAGECOUNT 2
#define PS_MSG_PRERROR 6
#define PS_MSG_FLUSHING 7
#define PS_MSG_OTHER 88
#define PS_MSG_MALFORMED 99

extern long ps_cookie;
extern long ps_pagecount;
extern int ps_flushing;
extern char ps_error[];

void psinit(void);
int psecho(int fd, int cookie);
int pscount(int fd, int cookie);
int psparse(const char *msg);
int pschar(char c);

#endif // _PS_H_
