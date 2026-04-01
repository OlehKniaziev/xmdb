#include "config.hpp"
#include <cstdlib>
#include "Result.hpp"
#include "util.hpp"

namespace xmdb
{
// @Safety All of these should be in read-only pages after the load succeeds.
#define X(name, _h, type, _ht) type g_##name;
XMDB_ENUM_CONFIG_OPTIONS
#undef X

template <typename T>
Result<T, ok::String> load_option(MallocAllocator *, const char *, const char *,
                                  const char *) = delete;

template <>
Result<ok::StringView, ok::String> load_option<ok::StringView>(
        MallocAllocator *allocator, const char *option_env_name,
        const char *option_human_name, const char *option_type_str)
{
    (void) option_human_name;
    (void) option_type_str;
    if (const char *option_value = getenv(option_env_name))
    {
        return ok::StringView{option_value};
    }
    else
    {
        return ok::String::format(allocator,
                                  "the '%s' environment variable is not set",
                                  option_env_name);
    }
}

ok::Optional<ok::String> GlobalConfig::load()
{
    MallocAllocator allocator{};

#define X(name, human_name, type, human_type)                                  \
    auto opt_res =                                                             \
            load_option<type>(&allocator, #name, #human_name, #human_type);    \
    if (opt_res.ok())                                                          \
    {                                                                          \
        g_##name = opt_res.unwrap();                                           \
    }                                                                          \
    else                                                                       \
    {                                                                          \
        return ok::String::format(                                             \
                &allocator, "failed to load option '" #human_name "': %s",     \
                opt_res.error().cstr());                                       \
    }
    XMDB_ENUM_CONFIG_OPTIONS
#undef X

    return {};
}

#undef X

#define X(name, human_name, type, _ht)                                         \
    type GlobalConfig::get_##human_name()                                      \
    {                                                                          \
        return g_##name;                                                       \
    }
XMDB_ENUM_CONFIG_OPTIONS
#undef X
} // namespace xmdb
