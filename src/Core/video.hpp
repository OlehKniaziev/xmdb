#pragma once

#include "ok.hpp"
#include "Result.hpp"

#include <Plugin/Plugin.hpp>

namespace xmdb {
struct VideoContainer {
    ok::Slice<U8> data; // IDK honestly
};

// What should this struct have?
struct VideoFrame {
    ok::Slice<U8> data;
};

class VideoPlugin;

class VideoStream {
public:
    ok::Optional<VideoFrame> pull_sync();

    void stop();

    ~VideoStream() {
        stop();
    }

private:
    VideoPlugin *m_plugin;
    void *m_plugin_stream;
    U32 m_width;
    U32 m_height;
    U32 m_frame_rate;
};

struct StartVideoStreamParams {
    using Flags = U16;
    enum : Flags {
        F_DECOMPRESS = 1 << 0,
    };

    Flags flags;
};

class MediaSink {
};

class MediaTransform;

class MediaStream {
public:
    MediaStream* connect(MediaTransform* transform);

    void set_sink(MediaSink *consumer);

private:

};

class MediaTransform : public MediaStream {
};

class PipelineElement {
public:
    virtual ok::Slice<MediaSink *> inputs() = 0;

    virtual ok::Slice<MediaStream *> outputs() = 0;

    // NOTE(oleh): Should be in a subclass or something.
    using OnNewStreamHook = void (*)(MediaStream *stream, void *custom_data);

    virtual void on_new_stream(OnNewStreamHook hook, void *data) = 0;

private:
};

class Pipeline {
public:
    void start();

    void add(PipelineElement *element);

private:
    VideoPlugin *m_plugin;
};

class VideoPlugin {
public:
    Result<VideoPlugin, ok::String> from_raw(ok::Allocator *allocator,
                                             plugin::Plugin *plug);

    Result<Pipeline, ok::String> create_pipeline();

private:
    // TODO(oleh): Add more stuff like
    //             - load_video_container (?)
    //             - is_container_supported (?)
    struct VideoPluginCapabilities {
        plugin::PluginCapability create_pipeline;
        plugin::PluginCapability pull_sync;
        plugin::PluginCapability pull_async;
        plugin::PluginCapability set_source;
        plugin::PluginCapability get_streams;
        plugin::PluginCapability start_stream;
        plugin::PluginCapability stop_stream;
        plugin::PluginCapability get_transforms;
    };

    VideoPlugin(ok::Allocator *allocator,
                plugin::Plugin *plugin,
                VideoPluginCapabilities caps) : m_allocator{allocator},
                                                m_plugin{plugin},
                                                m_caps{caps} {
    }

    ok::Allocator *m_allocator;
    plugin::Plugin *m_plugin;
    VideoPluginCapabilities m_caps;
};
} // namespace xmdb
