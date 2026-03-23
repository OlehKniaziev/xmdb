#include "video.hpp"

#include <Core/new.hpp>

namespace xmdb {
using namespace ok::literals;
using namespace xmdb::plugin;

Result<VideoPlugin*, ok::String> VideoPlugin::from_raw(ok::Allocator *allocator,
                                                       plugin::Plugin *plug) {
#define X(cap_name) \
    auto cap_name##_cap = plug->get_capability(ok::StringView{#cap_name}); \
    if (!cap_name##_cap) { \
    return ok::String::alloc(allocator, \
    "The plugin is missing the '" #cap_name "' capability"); \
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

void run_stuff(VideoPlugin *plugin, MediaSource *media_source) {
    Pipeline *pipeline = plugin->create_pipeline().unwrap();
    Demux *source = new Demux{media_source};

    source->on_new_stream([](MediaStream *stream, void *data) {
        auto *pipeline = static_cast<Pipeline *>(data);

        if (stream->type() == MediaType::VIDEO) {
            auto *video_consumer = new VideoConsumer([](VideoConsumer *consumer, VideoFrame *frame, void *data) {
                (void) consumer;
                (void) frame;
                (void) data;
                // USE(data);
            }, nullptr);

            pipeline->connect(stream, video_consumer);
        }
    }, pipeline);
}
} // namespace xmdb
