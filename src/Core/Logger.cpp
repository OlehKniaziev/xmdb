#include "Logger.hpp"

namespace xmdb {
void FileLogger::log(LogLevel lvl, const char *fmt, va_list args) {
    if (log_level > lvl) return;

    switch (lvl) {
    case LogLevel::INFO:
        fprintf(file, "[INFO] ");
        break;
    case LogLevel::WARN:
        fprintf(file, "[WARN] ");
        break;
    case LogLevel::ERROR:
        fprintf(file, "[ERROR] ");
        break;
    case LogLevel::NONE:
        OK_UNREACHABLE();
    }

    vfprintf(file, fmt, args);
    fprintf(file, "\n");
}

static FileLogger global_logger_impl{LogLevel::INFO, stdout};
static Logger *global_logger = &global_logger_impl;

void set_log_level(LogLevel lvl) {
    global_logger->set_log_level(lvl);
}

namespace log {
#define LOG(lvl) do { \
    va_list args; \
    va_start(args, fmt); \
    global_logger->log(lvl, fmt, args); \
    va_end(args); \
    } while (false)

void log(LogLevel lvl, const char *fmt, ...) {
    LOG(lvl);
}

void info(const char *fmt, ...) {
    LOG(LogLevel::INFO);
}

void warn(const char *fmt, ...) {
    LOG(LogLevel::WARN);
}

void error(const char *fmt, ...) {
    LOG(LogLevel::ERROR);
}
}
}
