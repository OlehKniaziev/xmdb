#pragma once

#include "ok.hpp"
#include "Result.hpp"

#include <Plugin/Plugin.hpp>

namespace xmdb {
enum class MediaSourceFormat {
    MP4,
};

struct BufferedStream {
    BufferedStream() = default;

    enum class Type {
        NETWORK,
        DISK,
    };

    // Num of bytes or an error string.
    Result<UZ, ok::String> pull(ok::Allocator *allocator);

    Type type;
    ok::Slice<U8> buffer;

    union {
        ok::File file;
        // NetworkStream? NetworkHandle? Raw socket? What is a network stream here?
        // HTTP streaming? Abstract class?
        void *network;
    } u;
};

class MediaSource {
public:
    static MediaSource from_slice(ok::Slice<U8> slice);

    bool is_in_memory();

    ok::Slice<U8> get_buffer();

    BufferedStream *get_stream();

    MediaSourceFormat identify_format();

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

class MediaSink {
};

enum class MediaType {
    VIDEO,
    AUDIO,
};

class MediaTransform;

class Pipeline;

class MediaStream;

class PipelineElement {
public:
    virtual ok::Slice<MediaSink *> inputs() = 0;

    virtual ok::Slice<MediaStream *> outputs() = 0;

protected:
    // explicit PipelineElement(Pipeline *pipeline) : m_pipeline{pipeline} {}

    // Pipeline *m_pipeline;
};

class MediaStream : public PipelineElement {
public:
    explicit MediaStream(MediaSource *source) : m_source{source} {}

    void set_sink(MediaSink *consumer);

    MediaType type() const;

    ok::Slice<MediaSink *> inputs() override;
    ok::Slice<MediaStream *> outputs() override;

private:
    MediaSource *m_source;
};

class MediaTransform : public MediaStream {
};

class Pull : public PipelineElement {
public:
    ok::Optional<VideoFrame> pull_sync();

private:
    MediaStream *m_stream;
};

class VideoConsumer : public PipelineElement {
public:
    using Hook = void (*)(VideoConsumer *consumer, VideoFrame *frame, void *data);

    VideoConsumer(Hook hook, void *data) : m_hook{hook}, m_data{data} {}

    ok::Slice<MediaStream *> outputs() override;
    ok::Slice<MediaSink *> inputs() override;

private:
    Hook m_hook;
    void *m_data;
};

class Demux : public PipelineElement {
public:
    explicit Demux(MediaSource *source) : m_source{source} {}

    ok::Slice<MediaSink *> inputs() override;
    ok::Slice<MediaStream *> outputs() override;

    using OnNewStreamHook = void (*)(MediaStream *stream, void *custom_data);

    virtual void on_new_stream(OnNewStreamHook hook, void *data);

private:
    MediaSource *m_source;
    ok::Optional<OnNewStreamHook> m_on_new_stream_hook;
};

class Pipeline {
public:
    void start();

    void add(PipelineElement *element);

    Result<int /* TBD */, ok::String> connect(PipelineElement *source, PipelineElement *dest);

private:
    VideoPlugin *m_plugin;
};

#define XMDB_ENUM_MEDIA_PLUGIN_CAPABILITIES \
    X(stream_pull_sync) \
    X(stream_pull_async) \
    X(pipeline_create)

class VideoPlugin {
public:
    Result<VideoPlugin *, ok::String> from_raw(ok::Allocator *allocator,
                                               plugin::Plugin *plug);

    Result<Pipeline *, ok::String> create_pipeline();

private:
    // TODO(oleh): Add more stuff like
    //             - load_video_container (?)
    //             - is_container_supported (?)
    struct VideoPluginCapabilities {
#define X(cap) plugin::PluginCapability cap;
        XMDB_ENUM_MEDIA_PLUGIN_CAPABILITIES
#undef X
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

Result<VideoPlugin *, ok::String> video_plugin_for(MediaSourceFormat format);
} // namespace xmdb
