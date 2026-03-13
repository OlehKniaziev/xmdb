#pragma once

#include <Core/ok.hpp>
#include <Core/util.hpp>

namespace xmdb::argparser {
enum class Flag {
    STRING,
    INT,
    BOOL,
};

struct FlagSpec {
    const char *name;
    Flag type;
    void *dest;
    const void *default_value;
};

class ArgParser {
public:
    ArgParser(int argc, char **argv, ok::Allocator *allocator = nullptr) : m_argc{argc}, m_argv{argv} {
        OK_VERIFY(m_argc > 0);

        if (allocator == nullptr) {
            m_allocator = new MallocAllocator{};
        }

        m_flag_specs = ok::List<FlagSpec>::alloc(allocator);
        m_positionals = ok::List<const char *>::alloc(allocator);
    }

    ArgParser &string(const char *name, const char **dest, const char *default_value = nullptr);
    ArgParser &integer(const char *name, S64 *dest, ok::Optional<S64> default_value = {});
    ArgParser &boolean(const char *name, bool *dest);

    bool parse();

    ok::Slice<const char *> positionals();

    ok::StringView error_message();

    void help();

private:
    void arg(const char *name, Flag type, void *dest, const void *default_value) {
        m_flag_specs.push(FlagSpec{.name = name, .type = type, .dest = dest, .default_value = default_value});
    }

    int m_argc;
    char **m_argv;
    ok::Allocator *m_allocator;
    ok::List<FlagSpec> m_flag_specs;
    ok::List<const char *> m_positionals;
    ok::StringView m_error_message;
    bool m_failed;
};
} // namespace xmdb::argparser
