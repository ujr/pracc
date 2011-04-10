#include <alloca.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int brutesearch(char *p, char *a)
{
  int i, j;
  int M = strlen(p);  /* pattern: p[0..M-1] */
  int N = strlen(a);  /* text:    a[0..N-1] */

  for (i=0, j=0; j<M && i<N; i++, j++)
  	if (a[i] != p[j]) { i-=j; j=-1; }
  return (j == M) ? i-M : i;
}

int kmpsearch(char *p, char *a)
{
  int i, j;
  int M = strlen(p);  /* pattern: p[0..M-1] */
  int N = strlen(a);  /* text:    a[0..N-1] */

  /* initnext */
  int *next = alloca(M*sizeof(int));
  printf("kmpsearch: next@%x, M=%d, N=%d\n", next, M, N);
  next[0] = -1;
  for (i=0, j=-1; i<M; i++, j++, next[i] = j) {
  	printf("kmpsearch: next[%d]=%d, j=%d", i, next[i], j); fflush(stdout);
  	while ((j>=0) && (p[i] != p[j])) {j = next[j]; printf(",%d", j);}
  	printf("\n");
  }
//  printf("kmpsearch: next ="); fflush(stdout);
//  for (i=0; i<M; i++) printf(" %d", next[i]);
//  printf("\n"); fflush(stdout);
  
  /* kmpsearch */
  for (i=0, j=0; j<M && i<N; i++, j++)
  	while ((j>=0) && (a[i] != p[j])) j = next[j];
  return (j == M) ? i-M : i;
}

int main(int argc, char **argv)
{
  char *p, *a;
  int i;

  if (argc != 3) return 127;
  p = *++argv;
  a = *++argv;

  i = kmpsearch(p, a);

  printf("%s\n", a);
  while (i--) putchar(' ');
  printf("%s\n", p);

  return 0;
}
