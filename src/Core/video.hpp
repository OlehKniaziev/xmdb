#pragma once

#include "BufferedStream.hpp"
#include "video-plugin-api.h"

#include <Plugin/Plugin.hpp>

namespace xmdb
{
enum class MediaSourceFormat
{
    MP4,
};

class VideoPlugin;

class MediaSource
{
public:
    static MediaSource from_slice(ok::Slice<U8> slice);

    bool is_in_memory() const
    {
        return m_in_memory;
    }

    ok::Slice<U8> get_buffer() const
    {
        OK_VERIFY(m_in_memory);
        return m_u.buffer;
    }

    BufferedStream *get_stream()
    {
        OK_VERIFY(!m_in_memory);
        return &m_u.buffered_stream;
    }

    Result<MediaSourceFormat, ok::String> identify_format(
            ok::Allocator *allocator, VideoPlugin *plugin);

private:
    explicit MediaSource(ok::Slice<U8> slice) :
        m_in_memory{true}, m_u{.buffer = slice}
    {
    }

    bool m_in_memory;
    union
    {
        ok::Slice<U8> buffer;
        BufferedStream buffered_stream;
    } m_u;
};

// What should this struct have?
struct VideoFrame
{
    int width;
    int height;
    ok::Slice<U8> data;
};

class VideoPlugin;

struct StartVideoStreamParams
{
    using Flags = U16;
    enum : Flags
    {
        F_DECOMPRESS = 1 << 0,
    };

    Flags flags;
};

class MediaSink
{
};

enum class MediaType
{
    VIDEO,
    AUDIO,
};

class MediaTransform;

class Pipeline;

class MediaStream;

class PipelineElement
{
public:
    friend Pipeline;

    virtual ok::Slice<MediaSink *> inputs() = 0;

    virtual ok::Slice<MediaStream *> outputs() = 0;

    Pipeline *pipeline;

    virtual ~PipelineElement()
    {
    }

protected:
    PipelineElement(Pipeline *pipeline, xmdb_PluginEntity plugin_state) :
        pipeline{pipeline}, m_plugin_state{plugin_state}
    {
    }

    xmdb_PluginEntity m_plugin_state;
};

class MediaStream : public PipelineElement
{
public:
    void set_sink(MediaSink *consumer);

    MediaType type() const;

    ok::Slice<MediaSink *> inputs() override;
    ok::Slice<MediaStream *> outputs() override;

private:
    MediaSource *m_source;
};

class MediaTransform : public MediaStream
{
};

class Pull : public PipelineElement
{
public:
    Pull() = delete;

    using Hook = void (*)(Pull *pull, VideoFrame *frame, void *data);

    static Result<Pull *, ok::String> create(ok::Allocator *allocator,
                                             Pipeline *pipeline);

    Result<bool, ok::String> pull_sync(VideoFrame *frame);

    ok::Slice<MediaStream *> outputs() override;
    ok::Slice<MediaSink *> inputs() override;

private:
    Pull(ok::Allocator *allocator, Pipeline *pipeline,
         xmdb_PluginEntity plugin_state) :
        PipelineElement{pipeline, plugin_state}, m_allocator{allocator}
    {
    }

    ok::Allocator *m_allocator;
};

class Demux : public PipelineElement
{
public:
    Demux() = delete;

    static Result<Demux *, ok::String> create(ok::Allocator *allocator,
                                              Pipeline *pipeline,
                                              MediaSource source);

    ok::Slice<MediaSink *> inputs() override;
    ok::Slice<MediaStream *> outputs() override;

    using OnNewStreamHook = void (*)(Demux *demux, MediaStream *stream,
                                     void *custom_data);

    void on_new_stream(OnNewStreamHook hook, void *data);

private:
    Demux(ok::Allocator *allocator, Pipeline *pipeline,
          xmdb_PluginEntity state) :
        PipelineElement{pipeline, state}, m_allocator{allocator}
    {
    }

    ok::Allocator *m_allocator;
};

class Pipeline
{
public:
    static Result<Pipeline *, ok::String> create(ok::Allocator *allocator,
                                                 VideoPlugin *plugin,
                                                 const char *name);

    void start();

    void add(PipelineElement *element, const char *name);

    ok::Optional<PipelineElement *> get_element(const char *name);

    ok::Optional<ok::String> connect(PipelineElement *source,
                                     PipelineElement *dest);

    VideoPlugin *plugin()
    {
        return m_plugin;
    }

private:
    Pipeline(ok::Allocator *allocator, VideoPlugin *plugin,
             xmdb_PluginEntity plugin_state) :
        m_allocator{allocator}, m_plugin{plugin}, m_plugin_entity{plugin_state}
    {
        m_name_to_element =
                ok::Table<ok::StringView, PipelineElement *>::alloc(allocator);
    }

    ok::Allocator *m_allocator;
    VideoPlugin *m_plugin;
    xmdb_PluginEntity m_plugin_entity;
    ok::Table<ok::StringView, PipelineElement *> m_name_to_element;
};

class VideoPlugin
{
public:
    static Result<VideoPlugin *, ok::String> from_raw(ok::Allocator *allocator,
                                                      plugin::Plugin *plug);

private:
    // TODO(oleh): Add more stuff like
    //             - load_video_container (?)
    //             - is_container_supported (?)
    struct VideoPluginCapabilities
    {
#define X(cap) plugin::PluginCapability cap;
        XMDB_ENUM_MEDIA_PLUGIN_CAPABILITIES
#undef X
    };

    VideoPlugin(ok::Allocator *allocator, plugin::Plugin *plugin,
                VideoPluginCapabilities caps) :
        m_allocator{allocator}, m_plugin{plugin}, m_caps{caps}
    {
    }

    friend Pipeline;
    friend Demux;
    friend Pull;
    friend MediaSource;

    ok::Allocator *m_allocator;
    plugin::Plugin *m_plugin;
    VideoPluginCapabilities m_caps;
};

Result<VideoPlugin *, ok::String> get_or_load_default_media_plugin(
        ok::Allocator *allocator);
} // namespace xmdb
