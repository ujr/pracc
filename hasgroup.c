#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>

/*
 * Check if the given group id (gid) is in the list of
 * the current process' (primary and auxiliary) groups.
 *
 * Return 1 if so, 0 if not-in-list, and -1 on error.
 */
int hasgroup(gid_t gid)
{
  gid_t *gidlist = {0};
  int i, num, found = 0;

  // Not all implementations of getgroup(2) include the primary
  // group in the list returned. Therefore, explicitly test if
  // the requested group equals the (real) primary group.

  if (getgid() == gid) return 1; // good

  // Now retrieve the list of groups and see if the requested
  // gid is an element of it. Carefully alloc the right amount
  // of memory to hold the entire list...

  num = getgroups(0, gidlist);
  if (num < 0) return -1; // errno

  gidlist = (gid_t *) calloc(num, sizeof(gid_t));
  if (gidlist == (gid_t *) 0) return -1; // ENOMEM

  num = getgroups(num, gidlist);
  if (num < 0) return -1; // errno

  for (i = 0; i < num; i++) {
  	if (gidlist[i] == gid) {
  		found = 1;
  		break;
  	}
  }

  free(gidlist);
  return found; // 1 if found, 0 if nil
}
