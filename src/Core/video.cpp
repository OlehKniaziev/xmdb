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

// NOTE(oleh): Is this even needed?
MediaSource MediaSource::from_slice(ok::Slice<U8> slice) {
    return MediaSource{slice};
}

static ok::String get_error(ok::Allocator *allocator, plugin::Plugin *plug) {
    ok::StringView error_message = plug->get_last_error().or_else(""_sv);
    return error_message.to_string(allocator);
}

ok::Optional<ok::String> Pipeline::connect(PipelineElement *source,
                                           PipelineElement *dest) {
    auto &connect_cap = m_plugin->m_caps.pipeline_connect;
    int ok = m_plugin->m_plugin->use_capability<int>(
            connect_cap, m_plugin_state, source->m_plugin_state,
            dest->m_plugin_state);

    if (!ok) {
        return get_error(m_allocator, m_plugin->m_plugin);
    }

    return {};
}

Result<Pipeline *, ok::String> Pipeline::create(ok::Allocator *allocator,
                                                VideoPlugin *plugin,
                                                const char *name) {
    auto &create_cap = plugin->m_caps.pipeline_create;
    void *out_pipeline = nullptr;
    int ok = plugin->m_plugin->use_capability<int>(create_cap, name,
                                                   &out_pipeline);

    if (!ok) {
        return get_error(allocator, plugin->m_plugin);
    }

    return new (allocator) Pipeline{
            allocator,
            plugin,
            out_pipeline,
    };
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

Result<Demux *, ok::String> Demux::create(ok::Allocator *allocator,
                                          Pipeline *pipeline,
                                          MediaSource source) {
    if (!source.is_in_memory()) {
        OK_TODO_MSG("Streaming media source");
    }

    ok::Slice<U8> source_buffer = source.get_buffer();

    auto *video_plugin = pipeline->plugin();
    auto &create_cap = video_plugin->m_caps.demux_create;

    void *plugin_demux = nullptr;

    int ok = video_plugin->m_plugin->use_capability<int>(
            create_cap, source_buffer.items, source_buffer.count,
            &plugin_demux);
    if (!ok) {
        return get_error(allocator, video_plugin->m_plugin);
    }

    return new (allocator) Demux{
            pipeline,
            plugin_demux,
    };
}

void Demux::on_new_stream(OnNewStreamHook hook, void *data) {
    (void) hook;
    (void) data;

    OK_TODO();
}

Result<Pull *, ok::String> Pull::create(ok::Allocator *allocator,
                                        Pipeline *pipeline, Hook hook,
                                        void *data) {
    auto *video_plugin = pipeline->plugin();
    auto &create_cap = video_plugin->m_caps.pull_create;

    struct DataStub {
        Hook hook;
        void *user_data;
    };

    auto *data_stub = new (allocator) DataStub{
            .hook = hook,
            .user_data = data,
    };

    void (*hook_stub)(Pull *, int, int, U8 *, DataStub *) =
            [](Pull *pull, int w, int h, U8 *frame_data, DataStub *data_stub) {
                VideoFrame frame{
                        .data = {frame_data, (UZ) (w * h)},
                };

                data_stub->hook(pull, &frame, data_stub->user_data);
            };

    void *pull_plugin = nullptr;

    int ok = video_plugin->m_plugin->use_capability<int>(
            create_cap, hook_stub, data_stub, &pull_plugin);

    if (!ok) {
        return get_error(allocator, video_plugin->m_plugin);
    }

    return new (allocator) Pull{
            pipeline,
            pull_plugin,
    };
}

void Pipeline::add(PipelineElement *element, const char *name) {
    auto &add_cap = m_plugin->m_caps.pipeline_add;

    m_plugin->m_plugin->use_capability<void>(add_cap, m_plugin_state,
                                             element->m_plugin_state, name);

    m_name_to_element.put(ok::StringView{name}, element);
}

ok::Optional<PipelineElement *> Pipeline::get_element(const char *name) {
    return m_name_to_element.get(ok::StringView{name});
}

ok::Slice<MediaStream *> Pull::outputs() {
    OK_TODO();
}

ok::Slice<MediaSink *> Pull::inputs() {
    OK_TODO();
}

ok::Slice<MediaStream *> Demux::outputs() {
    OK_TODO();
}

ok::Slice<MediaSink *> Demux::inputs() {
    OK_TODO();
}
} // namespace xmdb
