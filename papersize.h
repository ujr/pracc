#ifndef _PAPERSIZE_H_
#define _PAPERSIZE_H_

extern int
papersize_lookup_name(const char *name, float *width, float *height);

extern int
papersize_lookup_pcl5(int key, char **name, float *width, float *height);

extern int
papersize_lookup_pcl6(int key, char **name, float *width, float *height);

extern float
papersize_lookup_area(float area, char **name, float *width, float *height);

#endif // _PAPERSIZE_H_
