#include <string.h>

#define COPY_PROP_BUFSIZ 512
#ifndef MIN
#define MIN(x, y) (((x) < (y)) ? (x) : (y))
#endif

void copy_prop (char *dest, char *src, int len, int idx, int num_itm);
