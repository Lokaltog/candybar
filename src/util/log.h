#include <libgen.h>
#include <pthread.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
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

void LOG (const char *prefix, const char *color, const char *func, char *file, const char *format, ...);

#define LOG_ERR(...)  LOG("ERROR", ANSI_ESC_RED, __func__, __FILE__, __VA_ARGS__)
#define LOG_INFO(...) LOG("INFO", ANSI_ESC_RESET, __func__, __FILE__, __VA_ARGS__)
#define LOG_WARN(...) LOG("WARNING", ANSI_ESC_YELLOW, __func__, __FILE__, __VA_ARGS__)

#ifdef DEBUG
#define LOG_DEBUG(...) LOG("DEBUG", ANSI_ESC_WHITE, __func__, __FILE__, __VA_ARGS__)
#else
#define LOG_DEBUG(...)
#endif
