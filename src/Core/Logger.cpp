#include "Logger.hpp"

namespace xmdb
{
void FileLogger::log(LogLevel lvl, const char *fmt, va_list args)
{
    if (log_level > lvl) return;

    switch (lvl)
    {
    case LogLevel::DEBUG: fprintf(file, "[DEBUG] "); break;
    case LogLevel::INFO:  fprintf(file, "[INFO] "); break;
    case LogLevel::WARN:  fprintf(file, "[WARN] "); break;
    case LogLevel::ERROR: fprintf(file, "[ERROR] "); break;
    case LogLevel::NONE:  OK_UNREACHABLE();
    }

    vfprintf(file, fmt, args);
    fprintf(file, "\n");
}

static bool global_logger_init = false;
static Logger *global_logger = nullptr;

static bool check_logger()
{
    if (global_logger == nullptr)
    {
        if (global_logger_init) return true;

#ifdef XMDB_TEST
        constexpr auto DEFAULT_LOG_LEVEL = LogLevel::DEBUG;
#else
        constexpr auto DEFAULT_LOG_LEVEL = LogLevel::INFO;
#endif // XMDB_TEST

        static FileLogger impl{DEFAULT_LOG_LEVEL, stdout};

        global_logger = &impl;
        global_logger_init = true;

        return false;
    }
    else
    {
        return false;
    }
}

void set_logger(Logger *logger)
{
    global_logger = logger;
    global_logger_init = true;
}

void set_log_level(LogLevel lvl)
{
    if (check_logger()) return;

    global_logger->set_log_level(lvl);
}

namespace log
{
#define LOG(lvl)                                                               \
    do                                                                         \
    {                                                                          \
        if (check_logger()) return;                                            \
        va_list args;                                                          \
        va_start(args, fmt);                                                   \
        global_logger->log(lvl, fmt, args);                                    \
        va_end(args);                                                          \
    }                                                                          \
    while (false)

void log(LogLevel lvl, const char *fmt, ...)
{
    LOG(lvl);
}

void debug(const char *fmt, ...)
{
    LOG(LogLevel::DEBUG);
}

void info(const char *fmt, ...)
{
    LOG(LogLevel::INFO);
}

void warn(const char *fmt, ...)
{
    LOG(LogLevel::WARN);
}

void error(const char *fmt, ...)
{
    LOG(LogLevel::ERROR);
}
} // namespace log
} // namespace xmdb
