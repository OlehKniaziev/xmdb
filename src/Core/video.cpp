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

MediaStream *MediaStream::wrap(ok::Allocator *allocator, Pipeline *pipeline,
                               xmdb_PluginEntity state)
{
    return new (allocator) MediaStream{
            pipeline,
            state,
    };
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

void Pipeline::start()
{
    auto &start_cap = m_plugin->m_caps.pipeline_start;
    m_plugin->m_plugin->use_capability<void>(
            start_cap, plugin_entity_to_callback(m_plugin_entity), 1, 2, true);
}

ok::Optional<ok::String> Pipeline::connect(PipelineElement *source,
                                           PipelineElement *dest)
{
    auto &connect_cap = m_plugin->m_caps.pipeline_connect;
    int ok = m_plugin->m_plugin->use_capability<int>(
            connect_cap, plugin_entity_to_callback(m_plugin_entity),
            plugin_entity_to_callback(source->m_plugin_state),
            plugin_entity_to_callback(dest->m_plugin_state));

    if (!ok)
    {
        return get_error(m_allocator, m_plugin->m_plugin);
    }

    return {};
}

ok::Optional<ok::String> Pipeline::connect(MediaStream *source,
                                           PipelineElement *dest)
{
    auto &connect_cap = m_plugin->m_caps.stream_connect;
    int ok = m_plugin->m_plugin->use_capability<int>(
            connect_cap, plugin_entity_to_callback(m_plugin_entity),
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

Result<Push *, ok::String> Push::create(ok::Allocator *allocator,
                                        Pipeline *pipeline)
{
    auto *video_plugin = pipeline->plugin();
    auto &create_cap = video_plugin->m_caps.push_create;
    xmdb_PluginEntity plugin_state{};
    int ok = video_plugin->m_plugin->use_capability<int>(create_cap,
                                                         &plugin_state);

    if (!ok)
    {
        return get_error(allocator, video_plugin->m_plugin);
    }

    return new (allocator) Push{
            allocator,
            pipeline,
            plugin_state,
    };
}

void Push::push(MediaSource source)
{
    if (!source.is_in_memory())
    {
        OK_TODO_MSG("Streaming media source");
    }

    ok::Slice<U8> source_buffer = source.get_buffer();

    auto *video_plugin = pipeline->plugin();
    auto &push_cap = video_plugin->m_caps.push_push;
    int ok = video_plugin->m_plugin->use_capability<int>(
            push_cap, plugin_entity_to_callback(m_plugin_state),
            source_buffer.items, source_buffer.count);

    OK_VERIFY(ok);
}

void Demux::on_new_stream(OnNewStreamHook hook, void *data)
{
    struct UserDataStub
    {
        Demux *this_demux;
        OnNewStreamHook hook;
        void *user_data;
    };

    xmdb_DemuxOnNewStreamCallback hook_stub = [](void *demux_state,
                                                 xmdb_PluginEntity stream_state,
                                                 void *user_data)
    {
        (void) demux_state;

        auto *stub = static_cast<UserDataStub *>(user_data);
        auto *media_stream =
                MediaStream::wrap(stub->this_demux->m_allocator,
                                  stub->this_demux->pipeline, stream_state);

        stub->hook(stub->this_demux, media_stream, stub->user_data);
    };

    auto *data_stub = new (m_allocator) UserDataStub{
            .this_demux = this,
            .hook = hook,
            .user_data = data,
    };

    auto *plug = pipeline->plugin();

    auto &on_new_stream_cap = plug->m_caps.demux_on_new_stream;

    plug->m_plugin->use_capability<void>(on_new_stream_cap, m_plugin_state.impl,
                                         hook_stub, data_stub);
}

Result<Pull *, ok::String> Pull::create(ok::Allocator *allocator,
                                        Pipeline *pipeline)
{
    auto *video_plugin = pipeline->plugin();
    auto &create_cap = video_plugin->m_caps.pull_create;
    xmdb_PluginEntity plugin_state{};
    int ok = video_plugin->m_plugin->use_capability<int>(create_cap,
                                                         &plugin_state);

    if (!ok)
    {
        return get_error(allocator, video_plugin->m_plugin);
    }

    return new (allocator) Pull{
            allocator,
            pipeline,
            plugin_state,
    };
}

void Pipeline::add(PipelineElement *element, const char *name)
{
    auto &add_cap = m_plugin->m_caps.pipeline_add;

    m_plugin->m_plugin->use_capability<void>(
            add_cap, plugin_entity_to_callback(this->m_plugin_entity),
            plugin_entity_to_callback(element->m_plugin_state));

    m_name_to_element.put(ok::StringView{name}, element);
}

ok::Optional<PipelineElement *> Pipeline::get_element(const char *name)
{
    return m_name_to_element.get(ok::StringView{name});
}

Result<bool, ok::String> Pull::pull_sync(VideoFrame *frame)
{
    auto *plug = pipeline->plugin();
    auto &pull_sync_cap = plug->m_caps.pull_pull_sync;

    auto res = plug->m_plugin->use_capability<xmdb_PullSyncResult>(
            pull_sync_cap, plugin_entity_to_callback(m_plugin_state),
            &frame->width, &frame->height, &frame->data.items,
            &frame->data.count);
    switch (res)
    {
    case XMDB_PULL_SYNC_OK:  return true;
    case XMDB_PULL_SYNC_EOS: return false;
    case XMDB_PULL_SYNC_ERR:
    {
        return get_error(m_allocator, plug->m_plugin);
    }
    default:
        return ok::String::format(
                m_allocator,
                "unexpected return from the 'pull_pull_sync' capability call: "
                "'%d' instead of valid xmdb_PullSyncResult value",
                res);
    }
}

ok::Slice<MediaStream *> Pull::outputs()
{
    OK_TODO();
}

ok::Slice<MediaSink *> Pull::inputs()
{
    OK_TODO();
}

ok::Slice<MediaStream *> Push::outputs()
{
    OK_TODO();
}

ok::Slice<MediaSink *> Push::inputs()
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
