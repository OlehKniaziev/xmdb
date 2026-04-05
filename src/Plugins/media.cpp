#include <Core/ok.hpp>
#include <Core/video-plugin-api.h>
#include <stdlib.h>

extern "C"
{
int xmdb_plugin_load(void **out_state)
{
    *out_state = NULL;
    return 1;
}

XMDB_MEDIA_DECLARE_PIPELINE_CREATE()
{
    OK_TODO();
}

XMDB_MEDIA_DECLARE_PIPELINE_CONNECT()
{
    OK_TODO();
}

XMDB_MEDIA_DECLARE_PIPELINE_ADD()
{
    OK_TODO();
}

XMDB_MEDIA_DECLARE_PULL_CREATE()
{
    OK_TODO();
}

XMDB_MEDIA_DECLARE_DEMUX_CREATE()
{
    OK_TODO();
}

XMDB_MEDIA_DECLARE_DEMUX_ON_NEW_STREAM()
{
    OK_TODO();
}

XMDB_MEDIA_DECLARE_IDENTIFY_FORMAT_BASE64()
{
    OK_TODO();
}

#define X(name) #name,
const char *g_caps_names[] = {XMDB_ENUM_MEDIA_PLUGIN_CAPABILITIES};
#undef X

#define X(name) reinterpret_cast<void *>(&name##_cap),
void *g_caps_syms[] = {XMDB_ENUM_MEDIA_PLUGIN_CAPABILITIES};
#undef X

constexpr int g_caps_count = sizeof(g_caps_syms) / sizeof(*g_caps_syms);

void xmdb_plugin_install(void *state, const char *name,
                         const char ***caps_names, void ***caps_syms,
                         int *caps_count)
{
    (void) state;
    (void) name;

    *caps_names = g_caps_names;
    *caps_syms = g_caps_syms;
    *caps_count = g_caps_count;
}

const char *xmdb_plugin_get_last_error(void *state)
{
    (void) state;
    OK_TODO();
}
}
