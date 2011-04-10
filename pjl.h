#ifndef _PJL_H_
#define _PJL_H_

#define PJL_MSG_COOKIE 1
#define PJL_MSG_PAGECOUNT 2
#define PJL_MSG_JOBSTART 3
#define PJL_MSG_JOBEND 4
#define PJL_MSG_PAGE 5
#define PJL_MSG_OTHER 88

extern long pjl_cookie;
extern long pjl_pagecount;
extern long pjl_jobnum;
extern long pjl_numpages;
extern long pjl_curpage;

void pjlinit(void);
int pjluel(int fd);
int pjlecho(int fd, int cookie);
int pjlcount(int fd);
int pjljob(int fd, int jobid, char *personality, char *display);
int pjleoj(int fd, int jobid);
int pjloff(int fd);
int pjlparse(const char *msg);
int pjlchar(char c);

#endif // _PJL_H_
