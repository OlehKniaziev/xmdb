#pragma once

#ifdef __cplusplus
extern "C"
{
#define XMDB_EXTERN extern "C"
#else
#define XMDB_EXTERN extern
#endif // __cplusplus

typedef enum
{
    XMDB_MEDIA_FORMAT_MP4,
} xmdb_MediaFormat;

typedef struct
{
    void *impl;
    unsigned int callback_offset;
    int indirect;
} xmdb_PluginEntity;

static inline void *plugin_entity_to_callback(xmdb_PluginEntity e)
{
    void *ptr = (void *) ((unsigned char *) e.impl + e.callback_offset);
    if (e.indirect)
    {
        return *(void **) ptr;
    }
    else
    {
        return ptr;
    }
}

#define XMDB_ENUM_MEDIA_PLUGIN_CAPABILITIES                                    \
    X(pipeline_create)                                                         \
    X(pipeline_connect)                                                        \
    X(pipeline_add)                                                            \
    X(demux_create)                                                            \
    X(demux_on_new_stream)                                                     \
    X(pull_create)                                                             \
    X(identify_format_base64)

typedef void (*xmdb_PullOnFrameCallback)(void *pull_state, int frame_width,
                                         int frame_height,
                                         unsigned char *frame_data,
                                         void *user_data);

typedef void (*xmdb_DemuxOnNewStreamCallback)(void *demux, void *stream_state,
                                              void *user_data);

#define XMDB_MEDIA_DECLARE_PIPELINE_CREATE()                                   \
    XMDB_EXTERN int pipeline_create_cap(void *plugin_state,                    \
                                        const char *pipeline_name,             \
                                        xmdb_PluginEntity *out_pipeline)

#define XMDB_MEDIA_DECLARE_PIPELINE_CONNECT()                                  \
    XMDB_EXTERN int pipeline_connect_cap(void *plugin_state,                   \
                                         void *pipeline_state,                 \
                                         void *source_state, void *dest_state)

#define XMDB_MEDIA_DECLARE_PIPELINE_ADD()                                      \
    XMDB_EXTERN void pipeline_add_cap(                                         \
            void *plugin_state, void *pipeline_state, void *element_state)

#define XMDB_MEDIA_DECLARE_PULL_CREATE()                                       \
    XMDB_EXTERN int pull_create_cap(                                           \
            void *plugin_state, xmdb_PullOnFrameCallback callback,             \
            void *user_data, xmdb_PluginEntity *out_pull)

#define XMDB_MEDIA_DECLARE_DEMUX_CREATE()                                      \
    XMDB_EXTERN int demux_create_cap(                                          \
            void *plugin_state, unsigned char *source_data,                    \
            unsigned int source_data_count, xmdb_PluginEntity *out_demux)

#define XMDB_MEDIA_DECLARE_DEMUX_ON_NEW_STREAM()                               \
    XMDB_EXTERN int demux_on_new_stream_cap(                                   \
            void *plugin_state, void *demux_state,                             \
            xmdb_DemuxOnNewStreamCallback callback, void *user_data)

#define XMDB_MEDIA_DECLARE_IDENTIFY_FORMAT_BASE64()                            \
    XMDB_EXTERN int identify_format_base64_cap(                                \
            void *plugin_state, unsigned char *data, unsigned int data_count,  \
            xmdb_MediaFormat *out_format)

#ifdef __cplusplus
}
#endif // __cplusplus
