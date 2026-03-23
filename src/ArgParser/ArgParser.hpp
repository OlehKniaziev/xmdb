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
    const char *description;
    Flag type;
    void *dest;
    const void *default_value;
};

struct PositionalSpec {
    U32 idx;
    const char *name;
};

class ArgParser {
public:
    ArgParser(int argc, char **argv, ok::Allocator *allocator = nullptr) : m_argc{argc}, m_argv{argv}, m_failed{false} {
        OK_VERIFY(m_argc > 0);

        if (allocator == nullptr) {
            m_allocator = new MallocAllocator{};
        } else {
            m_allocator = allocator;
        }

        m_flag_specs = ok::List<FlagSpec>::alloc(m_allocator);
        m_positionals = ok::List<const char *>::alloc(m_allocator);
        m_positional_specs = ok::List<PositionalSpec>::alloc(m_allocator);
    }

    ArgParser &positional(U32 idx, const char *name);

    ArgParser &string(const char *name, const char **dest, const char *description, const char *default_value = nullptr);
    ArgParser &integer(const char *name, S64 *dest, const char *description, ok::Optional<S64> default_value = {});
    ArgParser &boolean(const char *name, bool *dest, const char *description);

    bool parse();

    ok::Slice<const char *> positionals();

    ok::StringView error_message();

    void help();

    void dealloc();

    ~ArgParser() {
        dealloc();
    }

private:
    void flag(const char *name, Flag type, void *dest, const char *description, const void *default_value) {
        m_flag_specs.push(FlagSpec{.name = name, .description = description, .type = type, .dest = dest, .default_value = default_value});
    }

    int m_argc;
    char **m_argv;
    ok::Allocator *m_allocator;
    ok::List<FlagSpec> m_flag_specs;
    ok::List<PositionalSpec> m_positional_specs;
    ok::List<const char *> m_positionals;
    ok::String m_error_message;
    bool m_failed;
};
} // namespace xmdb::argparser
