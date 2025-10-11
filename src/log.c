#include "log.h"
#include <string.h>

log_level_t current_log_level = LOG_LEVEL_INFO;

#define COLOR_RESET   "\033[0m"
#define COLOR_RED     "\033[31m"
#define COLOR_YELLOW  "\033[33m"
#define COLOR_BLUE    "\033[34m"
#define COLOR_GREEN   "\033[32m"
#define COLOR_CYAN    "\033[36m"

void log_init(log_level_t level) {
    current_log_level = level;
}

void log_message(log_level_t level, const char* prefix, const char* format, ...) {

    if (level < current_log_level) {
        return;
    }

    time_t now;
    time(&now);
    struct tm* timeinfo = localtime(&now);

    const char* color = COLOR_RESET;
    switch (level) {
        case LOG_LEVEL_DEBUG:
            color = COLOR_CYAN;
            break;
        case LOG_LEVEL_INFO:
            color = COLOR_GREEN;
            break;
        case LOG_LEVEL_WARNING:
            color = COLOR_YELLOW;
            break;
        case LOG_LEVEL_ERROR:
            color = COLOR_RED;
            break;
    }

    printf("%s[%02d:%02d:%02d] %s%s ", 
           color,
           timeinfo->tm_hour, 
           timeinfo->tm_min, 
           timeinfo->tm_sec,
           prefix,
           COLOR_RESET);

    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);

    printf("\n");
    fflush(stdout);
}