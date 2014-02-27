#include "log.h"

void
wklog (char const *format, ...) {
#ifdef DEBUG
	va_list args;
	time_t rawtime;
	struct tm *date;

	time(&rawtime);
	date = localtime(&rawtime);

	fprintf(stderr, "[%02d:%02d:%02d] wkline: ", date->tm_hour, date->tm_min, date->tm_sec);
	va_start(args, format);
	vfprintf(stderr, format, args);
	va_end(args);
	fprintf(stderr, "\n");
#endif
}
