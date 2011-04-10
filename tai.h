#ifndef _TAI_H_
#define _TAI_H_

/*
 * Seconds since 1970-01-01 00:00:10 TAI plus 4611686018427387914
 *
 * TAISTAMP: @4000000042d77da2
 * UTCSTAMP: 2005-07-15 10:10:48 UTC
 */

#define TAIBYTES 8  // size in bytes of a stored TAI label
#define TAISHIFT 4611686018427387914ULL  // 1970-01-01 00:00:10 TAI
#define TAISTAMPLEN (1+TAIBYTES+TAIBYTES)  // chars in a TAI stamp

typedef unsigned long long uint64;
struct tai { uint64 x; };

void tainow(struct tai *now); // current TAI64 label
void taiload(char buf[TAIBYTES], struct tai *t); // load t from buf
void taistore(char buf[TAIBYTES], struct tai *t); // store t in buf

int taifmt(char s[TAISTAMPLEN], struct tai *tp); // format *tp
int taiscan(const char *s, struct tai *tp); // scan tai stamp from s

#define taiunix(tp) ((tp)->x - TAISHIFT) // convert TAI to Unix time
#define unixtai(tp,t) ((tp)->x = (t) + TAISHIFT) // Unix time to TAI
#define tainull(tp) ((tp)->x = 0ULL) // origin (far far in the past)
#define tailast(tp) ((tp)->x = 0ULL - 1ULL) // end of the world
#define taiequal(tpa,tpb) ((tpa)->x == (tpb)->x) // tpa equals tpb
#define taibefore(tpa,tpb) ((tpa)->x < (tpb)->x) // tpa before tpb
#define taiadd(tp, t) ((tp)->x += (unsigned long long) (t)) // seconds

#include <time.h>
time_t tailocal(struct tai *taip, struct tm *tmp); // to local Unix

#endif /* _TAI_H_ */
