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
    explicit MediaSource(ok::Slice<U8> slice) :
        m_in_memory{true}, m_u{.buffer = slice} {
    }

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

    virtual ~PipelineElement() {
    }

protected:
    PipelineElement(Pipeline *pipeline, void *plugin_state) :
        pipeline{pipeline}, m_plugin_state{plugin_state} {
    }

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

    static Result<Pull *, ok::String>
    create(ok::Allocator *allocator, Pipeline *pipeline, Hook hook, void *data);

    Result<U32, ok::String> block_until_completion();

    ok::Slice<MediaStream *> outputs() override;
    ok::Slice<MediaSink *> inputs() override;

private:
    Pull(Pipeline *pipeline, void *plugin_state) :
        PipelineElement{pipeline, plugin_state} {
    }
};

class Demux : public PipelineElement {
public:
    Demux() = delete;

    static Result<Demux *, ok::String>
    create(ok::Allocator *allocator, Pipeline *pipeline, MediaSource source);

    ok::Slice<MediaSink *> inputs() override;
    ok::Slice<MediaStream *> outputs() override;

    using OnNewStreamHook = void (*)(Demux *demux, MediaStream *stream,
                                     void *custom_data);

    void on_new_stream(OnNewStreamHook hook, void *data);

private:
    Demux(Pipeline *pipeline, void *state) : PipelineElement{pipeline, state} {
    }

    ok::Optional<OnNewStreamHook> m_on_new_stream_hook;
};

class Pipeline {
public:
    static Result<Pipeline *, ok::String>
    create(ok::Allocator *allocator, VideoPlugin *plugin, const char *name);

    void start();

    void add(PipelineElement *element, const char *name);

    ok::Optional<PipelineElement *> get_element(const char *name);

    ok::Optional<ok::String> connect(PipelineElement *source,
                                     PipelineElement *dest);

    VideoPlugin *plugin() {
        return m_plugin;
    }

private:
    Pipeline(ok::Allocator *allocator, VideoPlugin *plugin,
             void *plugin_state) :
        m_allocator{allocator}, m_plugin{plugin}, m_plugin_state{plugin_state} {
        m_name_to_element =
                ok::Table<ok::StringView, PipelineElement *>::alloc(allocator);
    }

    ok::Allocator *m_allocator;
    VideoPlugin *m_plugin;
    void *m_plugin_state;
    ok::Table<ok::StringView, PipelineElement *> m_name_to_element;
};

#define XMDB_ENUM_MEDIA_PLUGIN_CAPABILITIES                                    \
    X(stream_pull_sync)                                                        \
    X(stream_pull_async)                                                       \
    X(pipeline_create)                                                         \
    X(pipeline_connect)                                                        \
    X(pipeline_add)                                                            \
    X(demux_create)                                                            \
    X(demux_on_new_stream)                                                     \
    X(pull_create)

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
    friend Demux;
    friend Pull;

    ok::Allocator *m_allocator;
    plugin::Plugin *m_plugin;
    VideoPluginCapabilities m_caps;
};

Result<VideoPlugin *, ok::String> video_plugin_for(ok::Allocator *allocator,
                                                   MediaSourceFormat format);
} // namespace xmdb
