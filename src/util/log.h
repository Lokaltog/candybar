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

void LOG_ERR (char const *format, ...);
void LOG_INFO (char const *format, ...);
void LOG_WARN (char const *format, ...);
