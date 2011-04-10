int scantok(const char *s, const char *delim)
{ /* replace first delim char in s with '\0',
   * return number of non-delim chars + following delim chars */
  unsigned long n;
  register char *p = (char *) s;

  n = strcspn(p, delim);
  if (n == 0) return 0; /* no token */
  p += n; *p++ = '\0';
  return n + 1 + strspn(p, delim);
}

