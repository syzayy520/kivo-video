#include "video/subtitle_plane/abi/api_table_builder.hpp"

#include "video/subtitle_plane/abi/api_function_declarations.hpp"

namespace kivo::video::subtitle_plane::runtime {

bool host_is_little_endian() noexcept {
    const uint16_t value = 1;
    return *reinterpret_cast<const uint8_t*>(&value) == 1;
}

KivoSubtitlePlaneApiV1 make_api_table() noexcept {
    KivoSubtitlePlaneApiV1 api{};
    api.header.struct_size = sizeof(KivoSubtitlePlaneApiV1);
    api.header.abi_major = KIVO_P8_ABI_MAJOR;
    api.header.abi_minor = KIVO_P8_ABI_MINOR;
    api.header.abi_patch = KIVO_P8_ABI_PATCH;
    api.header.abi_revision = KIVO_P8_ABI_REVISION;
    api.create_plane = api_create_plane;
    api.close_plane = api_close_plane;
    api.destroy_plane = api_destroy_plane;
    api.begin_discovery = api_begin_discovery;
    api.set_source = api_set_source;
    api.end_embedded_stream = api_end_embedded_stream;
    api.query_command = api_query_command;
    api.build_frame = api_build_frame;
    api.release_frame = api_release_frame;
    api.acquire_snapshot = api_acquire_snapshot;
    api.copy_snapshot = api_copy_snapshot;
    api.release_snapshot = api_release_snapshot;
    api.copy_text_buffer = api_copy_text_buffer;
    api.copy_font_blob = api_copy_font_blob;
    api.subscribe_events = api_subscribe_events;
    api.unsubscribe_events = api_unsubscribe_events;
    api.get_metrics = api_get_metrics;
    api.compute_redacted_source_identity = api_compute_redacted_source_identity;
    api.classify_style_semantics = api_classify_style_semantics;
    return api;
}

}  // namespace kivo::video::subtitle_plane::runtime
