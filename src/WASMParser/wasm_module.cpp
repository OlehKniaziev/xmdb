extern "C" void *module_alloc(unsigned long size) {
    static unsigned long off = 0;
    void *ptr = (void *)off;
    off += size;
    return ptr;
}

extern "C" void console_log(const char *fmt, ...);
extern "C" void console_error(const char *fmt, ...);

#define STB_SPRINTF_IMPLEMENTATION
#include "stb_sprintf.h"

#define OK_VSNPRINTF stbsp_vsnprintf

#define OK_ALLOC_PAGE(sz) (module_alloc((sz)))
#define OK_DEALLOC_PAGE(page, sz)

#define OK_LOG console_log
#define OK_LOG_ERROR console_error

#define OK_NO_STDLIB
#define OK_IMPLEMENTATION
#include <Core/ok.hpp>

#include <SQL/util.hpp>
#include <SQL/Lexer.cpp>
#include <SQL/Parser.cpp>

extern "C" bool sql_parse_source(const char *source,
                                 UZ count,
                                 UZ *line,
                                 UZ *col,
                                 U8 *err_buf,
                                 UZ err_buf_count) {
    ok::ArenaAllocator arena{};

    ok::StringView source_sv{source, count};
    xmdb::SQL::Parser parser{&arena, source_sv};

    Optional<xmdb::SQL::Query> q = parser.query();

    if (!q.has_value()) {
        xmdb::SQL::Parser::Error error = parser.error.get();
        *line = error.location.line;
        *col = error.location.column;
        UZ string_count = ok::min(err_buf_count, error.message.count());
        ok::memcpy(err_buf, error.message.cstr(), string_count);
    }

    return q.has_value();
}
