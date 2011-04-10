int parseacct(const char *pattern, const char *buf, unsigned len, long *amount)
{ /* scan buf[len] for acct info, return 1 if found, else 0 */
  long sum = 0, coef, value;

  if (!pattern) return 0;

  for (;;) { char c = *pattern++;
  	if (c == '\0') { /* end of pattern */
  		if (isspace(*buf)) --len, ++buf;
  		if (len) return 0;
  		else goto found;
  	}
  	if (c == ' ') { /* space matches sequence of white space */
  		if (!isspace(*buf)) return 0; /* mismatch */
  		else while (isspace(*buf)) { ++buf; --len; }
  		continue;
  	}
  	if (c == '*') { /* star matches up to next char in pattern */
  		c = *pattern;
  		if (c == '\0') goto found;
  		for (;;) {
  			if (len == 0) return 0;
  			if (*buf == c) break;
  			++buf; --len;
  		}
  		continue;
  	}
  	if (c == '{') { /* match number and add to sum */
  		int n = scani(pattern, &coef);
  		if (n && (pattern[n] == '}')) {
  			pattern += n;
  			pattern += 1;
  			n = scani(buf, &value);
  			if (!n) return 0;
  			sum += coef * value;
  			buf += n; len -= n;
  		}
  		continue;
  	}
  	if (len == 0) return 0;
  	if (*buf != c) return 0;
  	++buf; --len;
  }

found:
  if (amount) *amount = sum;
  return 1; /* ok, pattern matched */
}
