#pragma once

#include "ok.hpp"

namespace xmdb
{
#define XMDB_ENUM_CONFIG_OPTIONS                                               \
    XD(XMDB_DEFAULT_MEDIA_PLUGIN_PATH, default_media_plugin_path,              \
       ok::StringView, string, "plugins/libxmdbmediaplugin.so"_sv)

enum class ConfigOption
{
#define X(n, _h, _t, _ht) n,
#define XD(n, _h, _t, _ht, _d) n,
    XMDB_ENUM_CONFIG_OPTIONS
#undef X
#undef XD
};

struct GlobalConfig
{
    static ok::Optional<ok::String> load();

#define X(_n, h, t, _ht) static t get_##h();
#define XD(_n, h, t, _ht, _d) X(_n, h, t, _ht)
    XMDB_ENUM_CONFIG_OPTIONS
#undef X
#undef XD
};
} // namespace xmdb
