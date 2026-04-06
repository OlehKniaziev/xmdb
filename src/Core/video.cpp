#include "video.hpp"

#include "new.hpp"

#include <Core/config.hpp>
#include <Plugin/PluginManager.hpp>
#include <base64.h>

namespace xmdb
{
using namespace ok::literals;
using namespace xmdb::plugin;

Result<VideoPlugin *, ok::String> VideoPlugin::from_raw(
        ok::Allocator *allocator, plugin::Plugin *plug)
{
    plug->install("media"_sv);

#define X(cap_name)                                                            \
    auto cap_name##_cap = plug->get_capability(ok::StringView{#cap_name});     \
    if (!cap_name##_cap.ok())                                                  \
    {                                                                          \
        return ok::String::format(allocator,                                   \
                                  "could not get the plugin capability: %s",   \
                                  cap_name##_cap.error().cstr());              \
    }
    XMDB_ENUM_MEDIA_PLUGIN_CAPABILITIES
#undef X

    return new (allocator) VideoPlugin{
            allocator,
            plug,
            VideoPluginCapabilities{
#define X(cap_name) .cap_name = cap_name##_cap.unwrap(),
                    XMDB_ENUM_MEDIA_PLUGIN_CAPABILITIES
#undef X
            },
    };
}

static ok::String get_error(ok::Allocator *allocator, plugin::Plugin *plug)
{
    ok::StringView error_message = plug->get_last_error().or_else(""_sv);
    return error_message.to_string(allocator);
}

Result<MediaSourceFormat, ok::String> MediaSource::identify_format(
        ok::Allocator *allocator, VideoPlugin *plugin)
{
    if (m_in_memory)
    {
        ok::Slice<U8> buffer = get_buffer();

        auto &identify_cap = plugin->m_caps.identify_format_base64;
        xmdb_MediaFormat format{};

        web_string_view buffer_sv = {
                .Items = buffer.items,
                .Count = buffer.count,
        };

        UZ buffer_base64_count = buffer_sv.Count * 8 / 6 + 3;
        // @Safety Passing a temp pointer to a plugin callback. Need to document
        // that it's unsafe to store that pointer.
        U8 *buffer_base64 = allocator->alloc<U8>(buffer_base64_count);

        WebBase64Encode(buffer_sv, buffer_base64, &buffer_base64_count);

        int ok = plugin->m_plugin->use_capability<int>(
                identify_cap, buffer_base64, buffer_base64_count, &format);
        if (!ok)
        {
            return get_error(allocator, plugin->m_plugin);
        }

        switch (format)
        {
        case XMDB_MEDIA_FORMAT_MP4: return MediaSourceFormat::MP4;
        }

        OK_UNREACHABLE();
    }
    else
    {
        OK_TODO_MSG("Identify streaming");
    }

    OK_UNREACHABLE();
}

// NOTE(oleh): Is this even needed?
MediaSource MediaSource::from_slice(ok::Slice<U8> slice)
{
    return MediaSource{slice};
}

ok::Optional<ok::String> Pipeline::connect(PipelineElement *source,
                                           PipelineElement *dest)
{
    auto &connect_cap = m_plugin->m_caps.pipeline_connect;
    int ok = m_plugin->m_plugin->use_capability<int>(
            connect_cap, m_plugin_state,
            plugin_entity_to_callback(source->m_plugin_state),
            plugin_entity_to_callback(dest->m_plugin_state));

    if (!ok)
    {
        return get_error(m_allocator, m_plugin->m_plugin);
    }

    return {};
}

Result<Pipeline *, ok::String> Pipeline::create(ok::Allocator *allocator,
                                                VideoPlugin *plugin,
                                                const char *name)
{
    auto &create_cap = plugin->m_caps.pipeline_create;
    xmdb_PluginEntity out_pipeline{};
    int ok = plugin->m_plugin->use_capability<int>(create_cap, name,
                                                   &out_pipeline);

    if (!ok)
    {
        return get_error(allocator, plugin->m_plugin);
    }

    return new (allocator) Pipeline{
            allocator,
            plugin,
            out_pipeline,
    };
}

Result<Demux *, ok::String> Demux::create(ok::Allocator *allocator,
                                          Pipeline *pipeline,
                                          MediaSource source)
{
    if (!source.is_in_memory())
    {
        OK_TODO_MSG("Streaming media source");
    }

    ok::Slice<U8> source_buffer = source.get_buffer();

    auto *video_plugin = pipeline->plugin();
    auto &create_cap = video_plugin->m_caps.demux_create;

    xmdb_PluginEntity plugin_state{};

    int ok = video_plugin->m_plugin->use_capability<int>(
            create_cap, source_buffer.items, source_buffer.count,
            &plugin_state);
    if (!ok)
    {
        return get_error(allocator, video_plugin->m_plugin);
    }

    return new (allocator) Demux{
            allocator,
            pipeline,
            plugin_state,
    };
}

void Demux::on_new_stream(OnNewStreamHook hook, void *data)
{
    struct UserDataStub
    {
        OnNewStreamHook hook;
        void *user_data;
    };

    auto hook_stub = [](void *demux_state, void *stream_state, void *user_data)
    {
        auto *stub = static_cast<UserDataStub *>(user_data);
        (void) stub;
        (void) demux_state;
        (void) stream_state;
        OK_TODO();
        // stub->hook(demux_state, stream_state, stub->user_data);
    };

    auto *data_stub = new (m_allocator) UserDataStub{
            .hook = hook,
            .user_data = data,
    };

    auto *plug = pipeline->plugin();

    auto &on_new_stream_cap = plug->m_caps.demux_on_new_stream;

    plug->m_plugin->use_capability<void>(on_new_stream_cap, m_plugin_state.impl,
                                         hook_stub, data_stub);
}

Result<Pull *, ok::String> Pull::create(ok::Allocator *allocator,
                                        Pipeline *pipeline, Hook hook,
                                        void *data)
{
    auto *video_plugin = pipeline->plugin();
    auto &create_cap = video_plugin->m_caps.pull_create;

    struct UserDataStub
    {
        Hook hook;
        void *user_data;
    };

    auto *data_stub = new (allocator) UserDataStub{
            .hook = hook,
            .user_data = data,
    };

    void (*hook_stub)(Pull *, int, int, U8 *, UserDataStub *) =
            [](Pull *pull, int w, int h, U8 *frame_data,
               UserDataStub *data_stub)
    {
        VideoFrame frame{
                .data = {frame_data, (UZ) (w * h)},
        };

        data_stub->hook(pull, &frame, data_stub->user_data);
    };

    xmdb_PluginEntity plugin_state{};

    int ok = video_plugin->m_plugin->use_capability<int>(
            create_cap, hook_stub, data_stub, &plugin_state);

    if (!ok)
    {
        return get_error(allocator, video_plugin->m_plugin);
    }

    return new (allocator) Pull{
            pipeline,
            plugin_state,
    };
}

void Pipeline::add(PipelineElement *element, const char *name)
{
    auto &add_cap = m_plugin->m_caps.pipeline_add;

    m_plugin->m_plugin->use_capability<void>(
            add_cap, plugin_entity_to_callback(this->m_plugin_state),
            plugin_entity_to_callback(element->m_plugin_state));

    m_name_to_element.put(ok::StringView{name}, element);
}

ok::Optional<PipelineElement *> Pipeline::get_element(const char *name)
{
    return m_name_to_element.get(ok::StringView{name});
}

ok::Slice<MediaStream *> Pull::outputs()
{
    OK_TODO();
}

ok::Slice<MediaSink *> Pull::inputs()
{
    OK_TODO();
}

ok::Slice<MediaStream *> Demux::outputs()
{
    OK_TODO();
}

ok::Slice<MediaSink *> Demux::inputs()
{
    OK_TODO();
}

Result<VideoPlugin *, ok::String> get_or_load_default_media_plugin(
        ok::Allocator *allocator)
{
    static plugin::PluginManager plug_manager{allocator};
    static VideoPlugin *video_plug = nullptr;

    if (video_plug == nullptr)
    {
        auto plugin_path = GlobalConfig::get_default_media_plugin_path();
        auto plug = CHECK(plug_manager.get_or_load(plugin_path));

        video_plug = CHECK(VideoPlugin::from_raw(allocator, plug));
    }

    return video_plug;
}
} // namespace xmdb
