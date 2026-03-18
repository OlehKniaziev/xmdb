#pragma once

#include "ok.hpp"

namespace xmdb {
/**
 * @brief Severity levels for logging.
 */
enum class LogLevel {
    DEBUG, ///< Fine-grained informational events that are most useful to debug an application.
    INFO,  ///< Informational messages that highlight the progress of the application at coarse-grained level.
    WARN,  ///< Potentially harmful situations.
    ERROR, ///< Error events that might still allow the application to continue running.
    NONE,  ///< Turns off logging.
};

/**
 * @brief Abstract base class for loggers.
 */
struct Logger {
    /**
     * @brief Sets the minimum log level for this logger.
     * @param level The log level to set.
     */
    virtual void set_log_level(LogLevel level) = 0;

    /**
     * @brief Logs a message with a specific level and format.
     * @param level The log level of the message.
     * @param fmt The format string.
     * @param args The variable arguments.
     */
    virtual void log(LogLevel level, const char *fmt, va_list args) = 0;
};

/**
 * @brief A logger that outputs to a FILE* handle.
 */
struct FileLogger : public Logger {
    /**
     * @brief Constructs a new FileLogger.
     * @param lvl The minimum log level.
     * @param file The file to log to.
     */
    FileLogger(LogLevel lvl, FILE *file) : log_level{lvl}, file{file} {
    }

    /**
     * @brief Sets the minimum log level.
     * @param lvl The log level to set.
     */
    void set_log_level(LogLevel lvl) override {
        log_level = lvl;
    }

    /**
     * @brief Logs a message to the file.
     * @param level The log level.
     * @param fmt The format string.
     * @param args The variable arguments.
     */
    void log(LogLevel level, const char *fmt, va_list args) override;

    LogLevel log_level; ///< The current minimum log level.
    FILE *file;          ///< The file pointer.
};

/**
 * @brief Sets the global log level.
 * @param level The log level to set.
 */
void set_log_level(LogLevel level);

/**
 * @brief Sets the global logger instance.
 * @param logger Pointer to the logger to use.
 */
void set_logger(Logger *logger);

/**
 * @brief Namespace containing logging functions.
 */
namespace log {
/**
 * @brief Logs a message with a specific level.
 * @param level The log level.
 * @param fmt The format string.
 * @param ... The format arguments.
 */
void log(LogLevel level, const char *fmt, ...) OK_ATTRIBUTE_PRINTF(2, 3);

/**
 * @brief Logs a debug message.
 * @param fmt The format string.
 * @param ... The format arguments.
 */
void debug(const char *fmt, ...) OK_ATTRIBUTE_PRINTF(1, 2);

/**
 * @brief Logs an info message.
 * @param fmt The format string.
 * @param ... The format arguments.
 */
void info(const char *fmt, ...) OK_ATTRIBUTE_PRINTF(1, 2);

/**
 * @brief Logs a warning message.
 * @param fmt The format string.
 * @param ... The format arguments.
 */
void warn(const char *fmt, ...) OK_ATTRIBUTE_PRINTF(1, 2);

/**
 * @brief Logs an error message.
 * @param fmt The format string.
 * @param ... The format arguments.
 */
void error(const char *fmt, ...) OK_ATTRIBUTE_PRINTF(1, 2);
} // namespace log

#define XMDB_FIXME(msg)                                                                                                \
    do {                                                                                                               \
    ::xmdb::log::warn("\x1B[38;5;208m%s:%d: [%s] FIXME: %s \x1B[0m", __FILE__, __LINE__, __FUNCTION__, (msg));                           \
    } while (false)
} // namespace xmdb
