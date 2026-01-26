#pragma once

#include "ok.hpp"

namespace xmdb {
enum class LogLevel {
    NONE,
    INFO,
    WARN,
    ERROR,
};

struct Logger {
    virtual void set_log_level(LogLevel) = 0;
    virtual void log(LogLevel, const char *, va_list) = 0;
};

struct FileLogger : public Logger {
    FileLogger(LogLevel lvl, FILE *file) : log_level{lvl}, file{file} {
    }

    void set_log_level(LogLevel lvl) override {
        log_level = lvl;
    }

    void log(LogLevel, const char *fmt, va_list) override;

    LogLevel log_level;
    FILE *file;
};

void set_log_level(LogLevel);
void set_logger(Logger *);

namespace log {
void log(LogLevel, const char *, ...) OK_ATTRIBUTE_PRINTF(2, 3);

void info(const char *fmt, ...) OK_ATTRIBUTE_PRINTF(1, 2);
void warn(const char *fmt, ...) OK_ATTRIBUTE_PRINTF(1, 2);
void error(const char *fmt, ...) OK_ATTRIBUTE_PRINTF(1, 2);
} // namespace log

#define XMDB_FIXME(msg)                                                                                                \
    do {                                                                                                               \
    ::xmdb::log::warn("\x1B[38;5;208m%s:%d: [%s] FIXME: %s \x1B[0m", __FILE__, __LINE__, __FUNCTION__, (msg));                           \
    } while (false)
} // namespace xmdb
