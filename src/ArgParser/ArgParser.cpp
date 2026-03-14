#include "ArgParser.hpp"

namespace xmdb::argparser {
ArgParser &ArgParser::string(const char *name, const char **dest, const char *description, const char *default_value) {
    arg(name, Flag::STRING, dest, description, default_value);
    return *this;
}

ArgParser &ArgParser::integer(const char *name, S64 *dest, const char *description, ok::Optional<S64> default_value_opt) {
    S64 *default_value = nullptr;
    if (default_value_opt) {
        default_value = m_allocator->alloc<S64>();
        *default_value = default_value_opt.get();
    }

    arg(name, Flag::INT, dest, description, default_value);
    return *this;
}

ArgParser &ArgParser::boolean(const char *name, bool *dest, const char *description) {
    arg(name, Flag::BOOL, dest, description, nullptr);
    return *this;
}

static const char *flag_name_with_prefix(ok::Allocator *allocator, const char *flag_name) {
    char *buf = allocator->alloc<char>(strlen(flag_name) + 2);
    sprintf(buf, "-%s", flag_name);
    return buf;
}

bool ArgParser::parse() {
    OK_VERIFY(m_argc > 0);

    bool result = true;
    UZ arg_count = m_argc - 1;
    char **args = m_argv + 1;

    ok::List<FlagSpec> flag_specs = m_flag_specs.copy(m_allocator);

    for (UZ i = 0; i < arg_count; ) {
        const char *arg = args[i];
        if (arg[0] != '-') {
            m_positionals.push(arg);
            ++i;
            continue;
        }

        for (UZ f = 0; f < flag_specs.count; ) {
            FlagSpec spec = flag_specs[f];

            if (strcmp(arg + 1, spec.name) == 0) {
                switch (spec.type) {
                case Flag::BOOL: {
                    *(bool *) spec.dest = true;
                    ++i;
                    break;
                }
                case Flag::INT: {
                    ++i;
                    if (i >= arg_count) {
                        m_error_message = ok::String::format(m_allocator, "'-%s' flag provided, but no value specified", spec.name);
                        result = false;
                        goto cleanup;
                    }

                    const char *flag_value = args[i];

                    S64 *dest = (S64 *) spec.dest;
                    if (!ok::parse_int64(ok::StringView{flag_value}, dest)) {
                        m_error_message = ok::String::format(m_allocator,
                                                             "failed to parse '%s' as a valid integer value for the flag '-%s'",
                                                             flag_value,
                                                             spec.name);
                        result = false;
                        goto cleanup;
                    }

                    break;
                }
                case Flag::STRING: {
                    ++i;
                    if (i >= arg_count) {
                        m_error_message = ok::String::format(m_allocator,
                                                             "'-%s' flag provided, but no value specified",
                                                             spec.name);
                        result = false;
                        goto cleanup;
                    }

                    const char *flag_value = args[i];

                    *(const char **) spec.dest = flag_value;

                    break;
                }
                }

                flag_specs.remove_at(f);
            } else {
                ++f;
            }
        }
    }

    for (UZ i = 0; i < flag_specs.count; ++i) {
        FlagSpec spec = flag_specs[i];
        if (spec.type != Flag::BOOL && spec.default_value == nullptr) {
            m_error_message = ok::String::format(m_allocator,
                                                 "'-%s' flag requires a value",
                                                 spec.name);
            result = false;
            goto cleanup;
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

cleanup:
    flag_specs.dealloc();
    m_failed = !result;
    return result;
}

ok::Slice<const char *> ArgParser::positionals() {
    return m_positionals.slice();
}

ok::StringView ArgParser::error_message() {
    if (!m_failed) {
        OK_PANIC("'error_message' called when there is no error");
    }

    return m_error_message.view();
}

void ArgParser::help() {
    U32 max_width = 0;

    printf("Expected usage of %s:\n", m_argv[0]);

    ok::List<ok::String> formatted_flag_specs = ok::List<ok::String>::alloc(m_allocator);

    for (UZ i = 0; i < m_flag_specs.count; ++i) {
        FlagSpec spec = m_flag_specs[i];
        ok::String formatted_spec = ok::String::alloc(m_allocator);
        const char *flag_name = flag_name_with_prefix(m_allocator, spec.name);
        formatted_spec.format_append("\t%s", flag_name);

        if (spec.default_value != nullptr) {
            formatted_spec.push('=');

            switch (spec.type) {
            case Flag::STRING: {
                formatted_spec.format_append("%s", (const char *) spec.default_value);
                break;
            }
            case Flag::INT: {
                formatted_spec.format_append("%ld", *(S64 *) spec.default_value);
                break;
            }
            case Flag::BOOL: break;
            }
        }

        if (formatted_spec.count() > max_width) {
            max_width = formatted_spec.count();
        }

        formatted_flag_specs.push(formatted_spec);
    }

    for (UZ i = 0; i < m_flag_specs.count; ++i) {
        U32 total_written = 0;
        FlagSpec spec = m_flag_specs[i];
        ok::String formatted_flag_spec = formatted_flag_specs[i];

        total_written += printf("%s", formatted_flag_spec.cstr());

        U32 pad = max_width - total_written;

        for (U32 i = 0; i < pad; ++i) {
            printf(" ");
        }

        total_written = max_width;

        if (spec.description != nullptr && strlen(spec.description) > 0) {
            constexpr U32 column_limit = 80;
            U32 desc_written = 0;
            const char *separator = " - ";
            const char *description = spec.description;

            U32 desc_len = strlen(description);

            total_written += printf("%s", separator);

            S32 write_quota = (S32) column_limit - (S32) total_written;

            if (write_quota <= 0) {
                // TODO(oleh): Find a better way to work around this case.
                printf("\n%s", spec.description);
            } else {
                while (true) {
                    U32 w = ok::min(write_quota, (U32) strlen(description));
                    printf("%.*s", (int) w, description);

                    description += w;
                    desc_written += w;

                    if (desc_written == desc_len) break;

                    printf("\n");
                    for (U32 i = 0; i < total_written; ++i) {
                        printf(" ");
                    }
                }
            }
        }

        printf("\n");
    }
}

void ArgParser::dealloc() {
    m_positionals.dealloc();
    m_flag_specs.dealloc();
    if (m_failed) {
        m_error_message.dealloc();
    }
}
} // namespace xmdb::argparser
