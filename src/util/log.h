#include <stdarg.h>
#include <stdio.h>
#include <time.h>

#define ANSI_ESC_RED     "\x1b[31m"
#define ANSI_ESC_GREEN   "\x1b[32m"
#define ANSI_ESC_YELLOW  "\x1b[33m"
#define ANSI_ESC_BLUE    "\x1b[34m"
#define ANSI_ESC_MAGENTA "\x1b[35m"
#define ANSI_ESC_CYAN    "\x1b[36m"
#define ANSI_ESC_WHITE   "\x1b[37m"
#define ANSI_ESC_BOLD    "\x1b[1m"
#define ANSI_ESC_RESET   "\x1b[0m"

void LOG (const char *prefix, const char *desc, const char *format, ...);

#define LOG_ERR(format, ...)  LOG(ANSI_ESC_BOLD ANSI_ESC_RED "ERROR: " ANSI_ESC_RESET ANSI_ESC_RED, NULL, format, ## __VA_ARGS__)
#define LOG_INFO(format, ...) LOG("", NULL, format, ## __VA_ARGS__)
#define LOG_WARN(format, ...) LOG(ANSI_ESC_BOLD ANSI_ESC_YELLOW "WARNING: " ANSI_ESC_RESET ANSI_ESC_YELLOW, NULL, format, ## __VA_ARGS__)

#define W_LOG_ERR(format, ...)  LOG(ANSI_ESC_BOLD ANSI_ESC_RED "ERROR: " ANSI_ESC_RESET ANSI_ESC_RED, widget->name, format, ## __VA_ARGS__)
#define W_LOG_INFO(format, ...) LOG("", widget->name, format, ## __VA_ARGS__)
#define W_LOG_WARN(format, ...) LOG(ANSI_ESC_BOLD ANSI_ESC_YELLOW "WARNING: " ANSI_ESC_RESET ANSI_ESC_YELLOW, widget->name, format, ## __VA_ARGS__)
