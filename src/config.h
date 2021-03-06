#ifndef _config_h_
#define _config_h_

#include <stdio.h>

typedef int (*config_cb_t)(const char *, const char *, void *);

extern int config_parse_file(const char *fn, config_cb_t cb, void *data);
extern int config_parse_stream(FILE *fp, config_cb_t cb, void *data);

extern int config_match_sect(const char *key, const char *sect, const char *sub);
extern int config_match_name(const char *key, const char *name);
extern long config_get_int(const char *name, const char *value);
extern int config_get_bool(const char *name, const char *value);
// consider: _get_strdup(), _get_pathname()

// Must be provided by referencing code:
extern void config_error(const char *fmt, ...);

#endif // _config_h_
