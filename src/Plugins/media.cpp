#include <Core/Logger.hpp>
#include <Core/ok.hpp>
#include <Core/rc.hpp>
#include <Core/video-plugin-api.h>

#include <gst/app/app.h>
#include <gst/gst.h>

extern "C"
{
int xmdb_plugin_load(void **out_state)
{
    *out_state = nullptr;
    return 1;
}

XMDB_MEDIA_DECLARE_PIPELINE_CREATE()
{
    (void) plugin_state;

    GstElement *pipeline = gst_pipeline_new(pipeline_name);
    OK_ASSERT(pipeline != nullptr);

    out_pipeline->impl = pipeline;
    out_pipeline->callback_offset = 0;
    return 1;
}

XMDB_MEDIA_DECLARE_PIPELINE_CONNECT()
{
    (void) plugin_state;
    (void) pipeline_state;

    auto *src = GST_ELEMENT(source_state);
    auto *dest = GST_ELEMENT(dest_state);

    OK_ASSERT(gst_element_link(src, dest) == true);

    return 1;
}

XMDB_MEDIA_DECLARE_PIPELINE_ADD()
{
    (void) plugin_state;

    auto *pipeline = static_cast<GstElement *>(pipeline_state);

    OK_VERIFY(gst_bin_add(GST_BIN(pipeline),
                          static_cast<GstElement *>(element_state)) == true);
}

XMDB_MEDIA_DECLARE_PIPELINE_START()
{
    auto *pipeline = GST_PIPELINE(pipeline_state);
    GstStateChangeReturn ret =
            gst_element_set_state(GST_ELEMENT(pipeline), GST_STATE_PLAYING);
    return ret != GST_STATE_CHANGE_FAILURE;
}

struct AppSinkNewSampleData
{
    xmdb_PullOnFrameCallback callback;
    void *user_data;
};

struct PluginAsyncPullState
{
    PluginAsyncPullState() = delete;

    PluginAsyncPullState(AppSinkNewSampleData *new_sample_data,
                         GstElement *app_sink) :
        new_sample_data{new_sample_data}, app_sink{app_sink}
    {
    }

    static constexpr int UNSET = -1;

    AppSinkNewSampleData *new_sample_data;
    GstElement *app_sink;
    int stream_width = UNSET;
    int stream_height = UNSET;

    ~PluginAsyncPullState()
    {
        delete new_sample_data;
        gst_object_unref(app_sink);
    }
};

XMDB_EXTERN GstFlowReturn app_sink_new_sample_stub(GstElement *sink,
                                                   gpointer user_data)
{
    auto *pull_state = static_cast<PluginAsyncPullState *>(user_data);

    GstSample *sample = gst_app_sink_pull_sample(GST_APP_SINK(sink));

    if (pull_state->stream_width == PluginAsyncPullState::UNSET ||
        pull_state->stream_height == PluginAsyncPullState::UNSET)
    {
        GstCaps *sample_caps = gst_sample_get_caps(sample);
        GstStructure *sample_structure = gst_caps_get_structure(sample_caps, 0);

        gst_structure_get_int(sample_structure, "width",
                              &pull_state->stream_width);
        gst_structure_get_int(sample_structure, "height",
                              &pull_state->stream_height);
    }

    GstBuffer *sample_buffer = gst_sample_get_buffer(sample);

    GstMapInfo map_info;
    gst_buffer_map(sample_buffer, &map_info, GST_MAP_READ);

    // NOTE(oleh): Not sure if this is even needed or correct?
    OK_ASSERT(map_info.size == static_cast<UZ>(pull_state->stream_width *
                                               pull_state->stream_height));

    pull_state->new_sample_data->callback(
            sink, pull_state->stream_width, pull_state->stream_height,
            map_info.data, pull_state->new_sample_data->user_data);

    gst_buffer_unmap(sample_buffer, &map_info);
    gst_sample_unref(sample);

    return GST_FLOW_OK;
}

XMDB_MEDIA_DECLARE_PULL_CREATE()
{
    GstElement *app_sink = gst_element_factory_make("appsink", nullptr);

    if (app_sink == nullptr)
    {
        return 0;
    }

    out_pull->impl = app_sink;
    out_pull->callback_offset = 0;
    out_pull->indirect = false;

    return 1;
}

XMDB_MEDIA_DECLARE_PUSH_CREATE()
{
    (void) plugin_state;

    GstElement *app_src = gst_element_factory_make("appsrc", nullptr);
    if (app_src == nullptr)
    {
        return 0;
    }

    out_push->impl = app_src;
    out_push->callback_offset = 0;
    out_push->indirect = false;

    return 1;
}

XMDB_MEDIA_DECLARE_PUSH_PUSH()
{
    (void) plugin_state;

    auto *appsrc_elem = static_cast<GstElement *>(push_state);
    auto *appsrc = GST_APP_SRC(appsrc_elem);

    GstBuffer *buffer =
            gst_buffer_new_allocate(nullptr, source_data_count, nullptr);
    if (buffer == nullptr)
    {
        return 0;
    }

    GstMapInfo map_info{};
    gboolean mapped = gst_buffer_map(buffer, &map_info, GST_MAP_WRITE);
    if (!mapped)
    {
        gst_buffer_unref(buffer);
        return 0;
    }

    memcpy(map_info.data, source_data, source_data_count);
    gst_buffer_unmap(buffer, &map_info);

    GstFlowReturn ret = gst_app_src_push_buffer(appsrc, buffer);
    return ret == GST_FLOW_OK;
}


#if 0
XMDB_EXTERN int create_pull_async(void *plugin_state,
                                  xmdb_PullOnFrameCallback callback,
                                  void *user_data, xmdb_PluginEntity *out_pull)
{
    OK_PANIC("The async pull API is not finished");

    (void) plugin_state;

    GstElement *app_sink = gst_element_factory_make("appsink", nullptr);

    if (app_sink == nullptr)
    {
        return 0;
    }

    g_object_set(G_OBJECT(app_sink), "emit-signals", TRUE, NULL);

    auto *new_sample_data = new AppSinkNewSampleData{
            .callback = callback,
            .user_data = user_data,
    };

    auto *pull_state = new PluginAsyncPullState{
            new_sample_data,
            app_sink,
    };

    g_signal_connect(app_sink, "new-sample",
                     G_CALLBACK(app_sink_new_sample_stub), pull_state);

    out_pull->impl = pull_state;
    out_pull->callback_offset = offsetof(typeof(*pull_state), app_sink);
    out_pull->indirect = true;

    return 1;
}
#endif // 0

XMDB_MEDIA_DECLARE_PULL_PULL_SYNC()
{
    (void) plugin_state;

    auto *appsink_elem = static_cast<GstElement *>(pull_state);
    auto *appsink = GST_APP_SINK(appsink_elem);

    GstSample *sample = gst_app_sink_pull_sample(appsink);

    if (sample == nullptr)
    {
        if (gst_app_sink_is_eos(appsink))
        {
            return XMDB_PULL_SYNC_EOS;
        }
        else
        {
            return XMDB_PULL_SYNC_ERR;
        }
    }

    GstCaps *caps = gst_sample_get_caps(sample);
    GstStructure *structure = gst_caps_get_structure(caps, 0);

    gst_structure_get_int(structure, "width", out_width);
    gst_structure_get_int(structure, "height", out_height);

    GstBuffer *sample_buffer = gst_sample_get_buffer(sample);

    GstMapInfo map_info{};
    gst_buffer_map(sample_buffer, &map_info, GST_MAP_READ);

    *out_data = map_info.data;
    *out_data_count = map_info.size;

    // NOTE(oleh): Not sure what to unref here tbh, the object ownership is very
    // confusing.
    xmdb::ScopeGuard guard{[&]() { gst_sample_unref(sample); }};

    return XMDB_PULL_SYNC_OK;
}

struct PluginDemuxState
{
    GstElement *qt_demux;

    ~PluginDemuxState()
    {
        gst_object_unref(qt_demux);
    }
};

XMDB_MEDIA_DECLARE_DEMUX_CREATE()
{
    (void) plugin_state;

    XMDB_FIXME("Cannot use the 'qtdecoder' element for any container format, "
               "need to "
               "identify the format first");

    GstElement *demux = gst_element_factory_make("qtdemux", nullptr);
    if (demux == nullptr)
    {
        return 0;
    }

    auto *impl = new PluginDemuxState{
            .qt_demux = demux,
    };

    out_demux->impl = impl;
    out_demux->callback_offset = offsetof(typeof(*impl), qt_demux);
    out_demux->indirect = true;

    return 1;
}

struct PadAddedData
{
    PluginDemuxState *demux_state;
    xmdb_DemuxOnNewStreamCallback callback;
    void *user_data;
};

XMDB_EXTERN void qt_demux_pad_added_stub(GstElement *qt_demux, GstPad *pad,
                                         void *user_data)
{
    (void) qt_demux;

    auto *data = static_cast<PadAddedData *>(user_data);
    xmdb_PluginEntity pad_entity = {
            .impl = pad,
            .callback_offset = 0,
            .indirect = false,
    };
    data->callback(data->demux_state, pad_entity, data->user_data);
}

XMDB_MEDIA_DECLARE_DEMUX_ON_NEW_STREAM()
{
    (void) plugin_state;

    auto *demux = static_cast<PluginDemuxState *>(demux_state);

    auto *data_stub = new PadAddedData{
            .demux_state = demux,
            .callback = callback,
            .user_data = user_data,
    };

    g_signal_connect(demux->qt_demux, "pad-added",
                     G_CALLBACK(qt_demux_pad_added_stub), data_stub);

    return 1;
}

XMDB_MEDIA_DECLARE_STREAM_CONNECT()
{
    (void) plugin_state;
    OK_TODO();
}

XMDB_MEDIA_DECLARE_IDENTIFY_FORMAT_BASE64()
{
    OK_TODO();
}

#define X(name) #name,
const char *g_caps_names[] = {XMDB_ENUM_MEDIA_PLUGIN_CAPABILITIES};
#undef X

#define X(name) reinterpret_cast<void *>(&name##_cap),
void *g_caps_syms[] = {XMDB_ENUM_MEDIA_PLUGIN_CAPABILITIES};
#undef X

constexpr int g_caps_count = sizeof(g_caps_syms) / sizeof(*g_caps_syms);

void xmdb_plugin_install(void *state, const char *name,
                         const char ***caps_names, void ***caps_syms,
                         int *caps_count)
{
    (void) state;

    int fake_argc = 1;
    auto *fake_arg0 = strdup(name);
    auto *fake_argv = new char *[fake_argc];
    fake_argv[0] = fake_arg0;

    gst_init(&fake_argc, &fake_argv);

    *caps_names = g_caps_names;
    *caps_syms = g_caps_syms;
    *caps_count = g_caps_count;
}

const char *xmdb_plugin_get_last_error(void *state)
{
    (void) state;
    return nullptr;
}
}
