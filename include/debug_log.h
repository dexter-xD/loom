#ifndef DEBUG_LOG_H
#define DEBUG_LOG_H

#include <QDebug>

// Debug logging macros that can be disabled for release builds
#ifdef NDEBUG
    // Release build - disable all debug logs
    #define DEBUG_LOG(...) do {} while(0)
    #define DEBUG_LOG_PLUGIN(...) do {} while(0)
    #define DEBUG_LOG_LUA(...) do {} while(0)
    #define DEBUG_LOG_SYNTAX(...) do {} while(0)
    #define DEBUG_LOG_EDITOR(...) do {} while(0)
#else
    // Debug build - enable all debug logs
    #define DEBUG_LOG(...) qDebug() << __VA_ARGS__
    #define DEBUG_LOG_PLUGIN(...) qDebug() << "[PLUGIN]" << __VA_ARGS__
    #define DEBUG_LOG_LUA(...) qDebug() << "[LUA]" << __VA_ARGS__
    #define DEBUG_LOG_SYNTAX(...) qDebug() << "[SYNTAX]" << __VA_ARGS__
    #define DEBUG_LOG_EDITOR(...) qDebug() << "[EDITOR]" << __VA_ARGS__
#endif

// Always show critical errors and warnings regardless of build type
#define LOG_ERROR(...) qCritical() << "[ERROR]" << __VA_ARGS__
#define LOG_WARNING(...) qWarning() << "[WARNING]" << __VA_ARGS__
#define LOG_INFO(...) qInfo() << "[INFO]" << __VA_ARGS__

#endif // DEBUG_LOG_H