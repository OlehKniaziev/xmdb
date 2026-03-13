#include "ArgParser.hpp"

namespace xmdb::argparser {
ArgParser &ArgParser::string(const char *name, const char **dest, const char *default_value) {
    arg(name, Flag::STRING, dest, default_value);
    return *this;
}

ArgParser &ArgParser::integer(const char *name, S64 *dest, ok::Optional<S64> default_value_opt) {
    S64 *default_value = nullptr;
    if (default_value_opt) {
        default_value = m_allocator->alloc<S64>();
        *default_value = default_value_opt.get();
    }

    arg(name, Flag::INT, dest, default_value);
    return *this;
}

ArgParser &ArgParser::boolean(const char *name, bool *dest) {
    arg(name, Flag::BOOL, dest, nullptr);
    return *this;
}

static const char *flag_name_with_prefix(ok::Allocator *allocator, const char *flag_name) {
    char *buf = allocator->alloc<char>(strlen(flag_name) + 2);
    sprintf(buf, "-%s", flag_name);
    return buf;
}

bool ArgParser::parse() {
    OK_VERIFY(m_argc > 0);

    UZ arg_count = m_argc - 1;
    char **args = ++m_argv;

    for (UZ i = 0; i < arg_count; ) {
        const char *arg = args[i];
        if (arg[0] != '-') {
            m_positionals.push(arg);
            ++i;
            continue;
        }

        for (UZ f = 0; f < m_flag_specs.count; ) {
            FlagSpec spec = m_flag_specs[f];

            const char *flag_name = flag_name_with_prefix(m_allocator, spec.name);

            if (strcmp(arg, flag_name) == 0) {
                switch (spec.type) {
                case Flag::BOOL: {
                    *(bool *) spec.dest = true;
                    ++i;
                    break;
                }
                case Flag::INT: {
                    ++i;
                    if (i >= arg_count) {
                        m_error_message = ok::String::format(m_allocator, "'%s' flag provided, but no value specified", flag_name).view();
                        m_failed = true;
                        return false;
                    }

                    const char *flag_value = args[i];

                    S64 *dest = (S64 *) spec.dest;
                    if (!ok::parse_int64(ok::StringView{flag_value}, dest)) {
                        m_error_message = ok::String::format(m_allocator,
                                                             "failed to parse '%s' as a valid integer value for the flag '%s'",
                                                             flag_value,
                                                             flag_name).view();
                        m_failed = true;
                        return false;
                    }

                    break;
                }
                case Flag::STRING: {
                    ++i;
                    if (i >= arg_count) {
                        m_error_message = ok::String::format(m_allocator,
                                                             "'%s' flag provided, but no value specified",
                                                             flag_name).view();
                        m_failed = true;
                        return false;
                    }

                    const char *flag_value = args[i];

                    *(const char **) spec.dest = flag_value;

                    break;
                }
                }

                m_flag_specs.remove_at(f);
            } else {
                ++f;
            }
        }
    }

    for (UZ i = 0; i < m_flag_specs.count; ++i) {
        FlagSpec spec = m_flag_specs[i];
        if (spec.type != Flag::BOOL && spec.default_value == nullptr) {
            const char *flag_name = flag_name_with_prefix(m_allocator, spec.name);

            m_error_message = ok::String::format(m_allocator,
                                                 "'%s' flag requires a value",
                                                 flag_name).view();
            m_failed = true;
            return false;
        }

        switch (spec.type) {
        case Flag::INT: {
            S64 default_value = *(S64 *) spec.default_value;
            *(S64 *) spec.dest = default_value;
            break;
        }
        case Flag::STRING: {
            const char *default_value = (const char *) spec.default_value;
            *(const char **) spec.dest = default_value;
            break;
        }
        case Flag::BOOL: {
            *(bool *) spec.dest = false;
            break;
        };
        }
    }

    return true;
}

ok::Slice<const char *> ArgParser::positionals() {
    return m_positionals.slice();
}

ok::StringView ArgParser::error_message() {
    if (!m_failed) {
        OK_PANIC("'error_message' called when there is no error");
    }

    return m_error_message;
}
} // namespace xmdb::argparser
