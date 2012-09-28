#ifndef _config_h_
#define _config_h_

#include <stdio.h>

typedef int (*config_cb_t)(const char *, const char *, void *);
extern int config_parse_file(const char *fn, config_cb_t cb, void *data);
extern int config_parse_stream(FILE *fp, config_cb_t cb, void *data);

extern long config_get_int(const char *name, const char *value);
extern int config_get_bool(const char *name, const char *value);
// consider: _get_strdup(), _get_pathname()

#endif // _config_h_
