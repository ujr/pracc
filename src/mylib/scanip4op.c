#include "scan.h"

/* Scan a dotted decimal IPv4 address, optionally
 * with a port appended as ' port' or ':port' or '%port'
 */
int scanip4op(const char *s, unsigned char ip[4], unsigned short *port)
{
  unsigned long u;
  unsigned int i, len = 0;

  if ((i = scanu(s, &u)) == 0 || (u > 255)) return 0;
  ip[0] = (unsigned char) u;  s += i;  len += i;
  
  if (*s != '.') return 0;
  s++, len++;
  
  if ((i = scanu(s, &u)) == 0 || (u > 255)) return 0;
  ip[1] = (unsigned char) u;  s += i;  len += i;
  
  if (*s != '.') return 0;
  s++, len++;
  
  if ((i = scanu(s, &u)) == 0 || (u > 255)) return 0;
  ip[2] = (unsigned char) u;  s += i;  len += i;
  
  if (*s != '.') return 0;
  s++, len++;
  
  if ((i = scanu(s, &u)) == 0 || (u > 255)) return 0;
  ip[3] = (unsigned char) u;  s += i; len += i;

  if (port && (*s == ' ' || *s == ':' || *s == '%')) { /* look for port */
  	if ((i = scanu(++s, &u)) == 0 || (u > 65535)) return 0;
	*port = (unsigned short) u; len += i + 1;
  }

  return len;
}
