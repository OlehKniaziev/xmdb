#pragma once

#include "BufferedStream.hpp"

#include <Plugin/Plugin.hpp>

namespace xmdb {
enum class MediaSourceFormat {
    MP4,
};

class MediaSource {
public:
    static MediaSource from_slice(ok::Slice<U8> slice);

    bool is_in_memory() const {
        return m_in_memory;
    }

    ok::Slice<U8> get_buffer() const {
        OK_VERIFY(m_in_memory);
        return m_u.buffer;
    }

    BufferedStream *get_stream() {
        OK_VERIFY(!m_in_memory);
        return &m_u.buffered_stream;
    }

    Result<MediaSourceFormat, ok::String> identify_format();

private:
    bool m_in_memory;
    union {
        ok::Slice<U8> buffer;
        BufferedStream buffered_stream;
    } m_u;
};

// What should this struct have?
struct VideoFrame {
    ok::Slice<U8> data;
};

class VideoPlugin;

struct StartVideoStreamParams {
    using Flags = U16;
    enum : Flags {
        F_DECOMPRESS = 1 << 0,
    };

    Flags flags;
};

class MediaSink {};

enum class MediaType {
    VIDEO,
    AUDIO,
};

class MediaTransform;

class Pipeline;

class MediaStream;

class PipelineElement {
public:
    friend Pipeline;

    virtual ok::Slice<MediaSink *> inputs() = 0;

    virtual ok::Slice<MediaStream *> outputs() = 0;

    Pipeline *pipeline;

protected:
    void *m_plugin_state;
};

class MediaStream : public PipelineElement {
public:
    void set_sink(MediaSink *consumer);

    MediaType type() const;

    ok::Slice<MediaSink *> inputs() override;
    ok::Slice<MediaStream *> outputs() override;

private:
    MediaSource *m_source;
};

class MediaTransform : public MediaStream {};

class Pull : public PipelineElement {
public:
    Pull() = delete;

    using Hook = void (*)(Pull *pull, VideoFrame *frame, void *data);

    Pull(Hook hook, void *data) : m_hook{hook}, m_data{data} {
    }

    Result<U32, ok::String> block_until_completion();

    ok::Slice<MediaStream *> outputs() override;
    ok::Slice<MediaSink *> inputs() override;

private:
    Hook m_hook;
    void *m_data;
};

class Demux : public PipelineElement {
public:
    explicit Demux(MediaSource source) : m_source{source} {
    }

    ok::Slice<MediaSink *> inputs() override;
    ok::Slice<MediaStream *> outputs() override;

    using OnNewStreamHook = void (*)(Demux *demux, MediaStream *stream,
                                     void *custom_data);

    void on_new_stream(OnNewStreamHook hook, void *data);

private:
    MediaSource m_source;
    ok::Optional<OnNewStreamHook> m_on_new_stream_hook;
};

class Pipeline {
public:
    static Result<Pipeline *, ok::String>
    create(ok::Allocator *allocator, VideoPlugin *plugin, const char *name);

    void start();

    void add(PipelineElement *element, const char *name);

    Result<PipelineElement *, ok::String> get_element(const char *name);

    ok::Optional<ok::String> connect(PipelineElement *source,
                                     PipelineElement *dest);

private:
    ok::Allocator *m_allocator;
    void *m_plugin_state;
    VideoPlugin *m_plugin;
    ok::Table<ok::StringView, PipelineElement *> m_name_to_element;
};

#define XMDB_ENUM_MEDIA_PLUGIN_CAPABILITIES                                    \
    X(stream_pull_sync)                                                        \
    X(stream_pull_async)                                                       \
    X(pipeline_create)                                                         \
    X(pipeline_connect)                                                        \
    X(pipeline_add)

class VideoPlugin {
public:
    Result<VideoPlugin *, ok::String> from_raw(ok::Allocator *allocator,
                                               plugin::Plugin *plug);

private:
    // TODO(oleh): Add more stuff like
    //             - load_video_container (?)
    //             - is_container_supported (?)
    struct VideoPluginCapabilities {
#define X(cap) plugin::PluginCapability cap;
        XMDB_ENUM_MEDIA_PLUGIN_CAPABILITIES
#undef X
    };

    VideoPlugin(ok::Allocator *allocator, plugin::Plugin *plugin,
                VideoPluginCapabilities caps) :
        m_allocator{allocator}, m_plugin{plugin}, m_caps{caps} {
    }

    friend Pipeline;

    ok::Allocator *m_allocator;
    plugin::Plugin *m_plugin;
    VideoPluginCapabilities m_caps;
};

Result<VideoPlugin *, ok::String> video_plugin_for(ok::Allocator *allocator,
                                                   MediaSourceFormat format);
} // namespace xmdb
