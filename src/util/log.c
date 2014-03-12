#include "log.h"

void
LOG (const char *prefix, const char *color, const char *func, const char *file, const char *format, ...) {
	va_list args;
	time_t rawtime;
	struct tm *date;

	time(&rawtime);
	date = localtime(&rawtime);
	fprintf(stderr, "%s%s[%02d:%02d:%02d] %s%s%s %s",
	        ANSI_ESC_RESET, ANSI_ESC_WHITE,
	        date->tm_hour, date->tm_min, date->tm_sec,
	        ANSI_ESC_BOLD, color,
	        prefix,
	        ANSI_ESC_RESET);
	va_start(args, format);
	vfprintf(stderr, format, args);
	va_end(args);
	if (strstr(file, "widgets/")) {
		fprintf(stderr, " %s%s(%s)",
		        ANSI_ESC_RESET, ANSI_ESC_WHITE,
		        basename(file));
	}
	fprintf(stderr, ANSI_ESC_RESET "\n");
}
