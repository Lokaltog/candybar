#include "log.h"

void
LOG (const char *prefix, const char *desc, const char *format, ...) {
	va_list args;
	time_t rawtime;
	struct tm *date;

	time(&rawtime);
	date = localtime(&rawtime);
	fprintf(stderr, "%s%s[%02d:%02d:%02d]",
	        ANSI_ESC_RESET, ANSI_ESC_WHITE,
	        date->tm_hour, date->tm_min, date->tm_sec);
	if (desc != NULL) {
		fprintf(stderr, " %s%s[%s]", ANSI_ESC_RESET, ANSI_ESC_BOLD, desc);
	}
	fprintf(stderr, "%s %s", ANSI_ESC_RESET, prefix);
	va_start(args, format);
	vfprintf(stderr, format, args);
	va_end(args);
	fprintf(stderr, ANSI_ESC_RESET "\n");
}
