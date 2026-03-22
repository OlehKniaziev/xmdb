#include "video.hpp"

namespace xmdb {
using namespace ok::literals;
using namespace xmdb::plugin;

constexpr ok::StringView START_STREAM_CAP_NAME = "start_stream"_sv;
constexpr ok::StringView STOP_STREAM_CAP_NAME = "stop_stream"_sv;
constexpr ok::StringView PULL_FROM_SYNC_STREAM_CAP_NAME = "pull_from_sync_stream"_sv;
constexpr ok::StringView PULL_FROM_ASYNC_STREAM_CAP_NAME = "pull_from_async_stream"_sv;
constexpr ok::StringView CREATE_PIPELINE_STREAM_CAP_NAME = "create_pipeline"_sv;

Result<VideoPlugin, ok::String> VideoPlugin::from_raw(ok::Allocator *allocator,
                                                      plugin::Plugin *plug) {
    ok::Optional<PluginCapability> start_cap = plug->get_capability(START_STREAM_CAP_NAME);
    if (!start_cap) {
        return ok::String::format(allocator,
                                  "The plugin is missing the '" OK_SV_FMT "' capability",
                                  OK_SV_ARG(START_STREAM_CAP_NAME));
    }

    ok::Optional<PluginCapability> stop_cap = plug->get_capability(STOP_STREAM_CAP_NAME);
    if (!stop_cap) {
        return ok::String::format(allocator,
                                  "The plugin is missing the '" OK_SV_FMT "' capability",
                                  OK_SV_ARG(STOP_STREAM_CAP_NAME));
    }

    ok::Optional<PluginCapability> pull_from_sync_cap = plug->get_capability(PULL_FROM_SYNC_STREAM_CAP_NAME);
    if (!pull_from_sync_cap) {
        return ok::String::format(allocator,
                                  "The plugin is missing the '" OK_SV_FMT "' capability",
                                  OK_SV_ARG(PULL_FROM_SYNC_STREAM_CAP_NAME));
    }

    ok::Optional<PluginCapability> pull_from_async_cap = plug->get_capability(PULL_FROM_ASYNC_STREAM_CAP_NAME);
    if (!pull_from_async_cap) {
        return ok::String::format(allocator,
                                  "The plugin is missing the '" OK_SV_FMT "' capability",
                                  OK_SV_ARG(PULL_FROM_ASYNC_STREAM_CAP_NAME));
    }

    ok::Optional<PluginCapability> create_pipeline_cap = plug->get_capability(PULL_FROM_ASYNC_STREAM_CAP_NAME);
    if (!create_pipeline_cap) {
        return ok::String::format(allocator,
                                  "The plugin is missing the '" OK_SV_FMT "' capability",
                                  OK_SV_ARG(CREATE_PIPELINE_CAP_NAME));
    }
    return VideoPlugin{
        allocator,
        plug,
        VideoPluginCapabilities{
            .start_stream = start_cap.get(),
            .stop_stream = stop_cap.get(),
            .pull_sync = pull_from_sync_cap.get(),
            .pull_async = pull_from_async_cap.get(),
            .create_pipeline = create_pipeline_cap.get(),
        },
    };
}

void run_stuff() {
    Pipeline *pipeline;
    DemuxSourceElement *source = SourceElement::demux(container);

    source->on_new_stream([](MediaStream *stream, void *data) {
        if (stream->type() == Type::VIDEO) {
            auto *video_consumer = VideoConsumer::create([](VideoConsumer *consumer, VideoFrame *frame) {
                use(frame);
            });
            stream->connect(video_consumer);
        }
    }, nullptr);
}

} // namespace xmdb
