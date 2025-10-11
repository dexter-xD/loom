#ifndef LOG_H
#define LOG_H

#include <stdio.h>
#include <stdarg.h>
#include <time.h>

typedef enum {
    LOG_LEVEL_DEBUG = 0,
    LOG_LEVEL_INFO = 1,
    LOG_LEVEL_WARNING = 2,
    LOG_LEVEL_ERROR = 3
} log_level_t;

extern log_level_t current_log_level;

void log_message(log_level_t level, const char* prefix, const char* format, ...);

#ifdef NDEBUG

    #define LOG_DEBUG(...) do {} while(0)
    #define LOG_FILE_WATCHER(...) do {} while(0)
    #define LOG_WEBSOCKET(...) do {} while(0)
    #define LOG_HTTP(...) do {} while(0)
#else

    #define LOG_DEBUG(...) log_message(LOG_LEVEL_DEBUG, "[DEBUG]", __VA_ARGS__)
    #define LOG_FILE_WATCHER(...) log_message(LOG_LEVEL_DEBUG, "[FILE WATCHER]", __VA_ARGS__)
    #define LOG_WEBSOCKET(...) log_message(LOG_LEVEL_DEBUG, "[WebSocket]", __VA_ARGS__)
    #define LOG_HTTP(...) log_message(LOG_LEVEL_DEBUG, "[HTTP]", __VA_ARGS__)
#endif

#define LOG_ERROR(...) log_message(LOG_LEVEL_ERROR, "[ERROR]", __VA_ARGS__)
#define LOG_WARNING(...) log_message(LOG_LEVEL_WARNING, "[WARNING]", __VA_ARGS__)
#define LOG_INFO(...) log_message(LOG_LEVEL_INFO, "[INFO]", __VA_ARGS__)

#define LOG_SERVER(...) log_message(LOG_LEVEL_INFO, "[SERVER]", __VA_ARGS__)

void log_init(log_level_t level);

#endif 