/* praccpath.c - part of pracc sources */
/* Copyright (c) 2006 by Urs-Jakob Rueetschi */

#include <stdlib.h>  /* for calloc */
#include <string.h>  /* for strcpy and strlen */

#include "pracc.h"

/*
 * Return a pointer to an malloc'ed string containing
 * the full path to the pracc file for acctname or NULL
 */
char *praccpath(const char *acctname)
{
  int n = strlen(PRACCDIR);
  int m = strlen(acctname);

  char *path = (char *) calloc(n+1+m+1, sizeof(char));
  if (path == (char *) 0) return (char *) 0; // ENOMEM

  strcpy(path, PRACCDIR);
  if (path[n-1] != '/') path[n++] = '/';
  strcpy(path+n, acctname);

  return path;
}

/* Note:
 * This is the place to make changes if it is ever decided
 * that pracc files should be kept somewhere else, e.g.,
 * in several directoryes like /var/pracc/a/asmith, etc.
 */
