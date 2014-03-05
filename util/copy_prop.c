#include "copy_prop.h"

void
copy_prop (char *dest, char *src, int len, int idx, int num_itm)
{
	if (num_itm <= 1) {
		strncpy(dest, src, MIN(len, COPY_PROP_BUFSIZ));
		dest[len] = '\0';
	}
	else {
		int pos = 0, cnt = 0;
		while (cnt < idx && cnt < (num_itm - 1) && pos < len) {
			pos += strlen(src + pos) + 1;
			cnt++;
		}
		if (cnt == (num_itm - 1)) {
			copy_prop(dest, src + pos, len - pos, 0, 1);
		}
		else {
			strncpy(dest, src + pos, COPY_PROP_BUFSIZ);
		}
	}
}
