#include "log.h"

void
LOG_ERR (char const *format, ...) {
	va_list args;
	time_t rawtime;
	struct tm *date;

	time(&rawtime);
	date = localtime(&rawtime);
	fprintf(stderr, ANSI_ESC_RESET ANSI_ESC_WHITE "[%02d:%02d:%02d] " ANSI_ESC_RED ANSI_ESC_BOLD "ERROR: " ANSI_ESC_RESET ANSI_ESC_RED, date->tm_hour, date->tm_min, date->tm_sec);
	va_start(args, format);
	vfprintf(stderr, format, args);
	va_end(args);
	fprintf(stderr, ANSI_ESC_RESET "\n");
}

void
LOG_INFO (char const *format, ...) {
	va_list args;
	time_t rawtime;
	struct tm *date;

	time(&rawtime);
	date = localtime(&rawtime);
	fprintf(stderr, ANSI_ESC_RESET ANSI_ESC_WHITE "[%02d:%02d:%02d] " ANSI_ESC_RESET, date->tm_hour, date->tm_min, date->tm_sec);
	va_start(args, format);
	vfprintf(stderr, format, args);
	va_end(args);
	fprintf(stderr, ANSI_ESC_RESET "\n");
}

void
LOG_WARN (char const *format, ...) {
	va_list args;
	time_t rawtime;
	struct tm *date;

	time(&rawtime);
	date = localtime(&rawtime);
	fprintf(stderr, ANSI_ESC_RESET ANSI_ESC_WHITE "[%02d:%02d:%02d] " ANSI_ESC_YELLOW ANSI_ESC_BOLD "WARNING: " ANSI_ESC_RESET ANSI_ESC_YELLOW, date->tm_hour, date->tm_min, date->tm_sec);
	va_start(args, format);
	vfprintf(stderr, format, args);
	va_end(args);
	fprintf(stderr, ANSI_ESC_RESET "\n");
}
