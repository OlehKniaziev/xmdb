#pragma once

#include "ok.hpp"

namespace xmdb
{
#define XMDB_ENUM_CONFIG_OPTIONS                                               \
    X(XMDB_DEFAULT_MEDIA_PLUGIN_PATH, default_media_plugin_path,               \
      ok::StringView, string)

enum class ConfigOption
{
#define X(n, _h, _t, _ht) n,
    XMDB_ENUM_CONFIG_OPTIONS
#undef X
};

struct GlobalConfig
{
    static ok::Optional<ok::String> load();

#define X(_n, h, t, _ht) static t get_##h();
    XMDB_ENUM_CONFIG_OPTIONS
#undef X
};
} // namespace xmdb
