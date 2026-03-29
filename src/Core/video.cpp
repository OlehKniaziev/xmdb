#include "video.hpp"

#include <Core/new.hpp>

namespace xmdb {
using namespace ok::literals;
using namespace xmdb::plugin;

Result<VideoPlugin *, ok::String>
VideoPlugin::from_raw(ok::Allocator *allocator, plugin::Plugin *plug) {
#define X(cap_name)                                                            \
    auto cap_name##_cap = plug->get_capability(ok::StringView{#cap_name});     \
    if (!cap_name##_cap) {                                                     \
        return ok::String::alloc(allocator,                                    \
                                 "The plugin is missing the '" #cap_name       \
                                 "' capability");                              \
    }
    XMDB_ENUM_MEDIA_PLUGIN_CAPABILITIES
#undef X

    return new (allocator) VideoPlugin{
            allocator,
            plug,
            VideoPluginCapabilities{
#define X(cap_name) .cap_name = cap_name##_cap.get(),
                    XMDB_ENUM_MEDIA_PLUGIN_CAPABILITIES
#undef X
            },
    };
}
} // namespace xmdb
