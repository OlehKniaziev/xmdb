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

Result<MediaSourceFormat, ok::String> MediaSource::identify_format() {
    OK_TODO_MSG("Check if GStreamer has something like this and architect this "
                "mofo after it");
}

ok::Optional<ok::String> Pipeline::connect(PipelineElement *source,
                                           PipelineElement *dest) {
    auto connect_cap = m_plugin->m_caps.pipeline_connect;
    int ok = m_plugin->m_plugin->use_capability<int>(
            connect_cap, m_plugin_state, source->m_plugin_state,
            dest->m_plugin_state);

    if (!ok) {
        auto err = m_plugin->m_plugin->get_last_error();
        if (err) return err.get().to_string(m_allocator);
        else
            return ok::String::alloc(m_allocator, "unknown");
    }

    return {};
}

U64 ok_hash_value(MediaSourceFormat format) {
    return static_cast<U64>(format);
}

static ok::Table<MediaSourceFormat, VideoPlugin *> registered_plugins{};

Result<VideoPlugin *, ok::String> video_plugin_for(ok::Allocator *allocator,
                                                   MediaSourceFormat format) {
    auto plug = registered_plugins.get(format);
    if (!plug) return ok::String::alloc(allocator, "no registerd plugin found");
    return plug.get();
}
} // namespace xmdb
