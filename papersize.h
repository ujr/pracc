#ifndef _PAPERSIZE_H_
#define _PAPERSIZE_H_

// All lookup functions return 1 if found and 0 if not found,
// except _area, which returns the nearest area.

extern int
papersize_lookup_name(const char *name, float *width, float *height);

extern int
papersize_lookup_pcl5(int key, const char **name, float *width, float *height);

extern int
papersize_lookup_pcl6(int key, const char **name, float *width, float *height);

extern float
papersize_lookup_area(float area, const char **name, float *width, float *height);

#endif // _PAPERSIZE_H_
