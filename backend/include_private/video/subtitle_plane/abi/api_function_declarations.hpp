#pragma once

#include "kivo/video/subtitle_plane/kivo_subtitle_plane.h"

namespace kivo::video::subtitle_plane::runtime {

KivoP8ApiStatus KIVO_P8_CALL api_create_plane(
    const KivoSubtitlePlaneConfig* config,
    KivoSubtitlePlaneHandle* out_plane) KIVO_P8_NOEXCEPT;
KivoP8ApiStatus KIVO_P8_CALL api_close_plane(KivoSubtitlePlaneHandle plane) KIVO_P8_NOEXCEPT;
KivoP8ApiStatus KIVO_P8_CALL api_destroy_plane(KivoSubtitlePlaneHandle plane) KIVO_P8_NOEXCEPT;
KivoP8ApiStatus KIVO_P8_CALL api_begin_discovery(
    KivoSubtitlePlaneHandle plane,
    const KivoSubtitleDiscoveryBeginRequest* request,
    KivoSubtitleCommandToken* out_token) KIVO_P8_NOEXCEPT;
KivoP8ApiStatus KIVO_P8_CALL api_set_source(
    KivoSubtitlePlaneHandle plane,
    const KivoSubtitleSourceDescriptor* source,
    KivoSubtitleCorrelationId correlation_id,
    KivoSubtitleCommandToken* out_token) KIVO_P8_NOEXCEPT;
KivoP8ApiStatus KIVO_P8_CALL api_end_embedded_stream(
    KivoSubtitlePlaneHandle plane,
    const KivoEmbeddedSubtitleStreamEndRequest* request,
    KivoSubtitleCommandToken* out_token) KIVO_P8_NOEXCEPT;
KivoP8ApiStatus KIVO_P8_CALL api_query_command(
    KivoSubtitlePlaneHandle plane,
    KivoSubtitleCommandId command_id,
    KivoSubtitleCommandLifecycleSnapshot* out_snapshot) KIVO_P8_NOEXCEPT;
KivoP8ApiStatus KIVO_P8_CALL api_build_frame(
    KivoSubtitlePlaneHandle plane,
    const KivoSubtitleFrameBuildRequest* request,
    KivoSubtitleFrameLease* out_lease) KIVO_P8_NOEXCEPT;
KivoP8ApiStatus KIVO_P8_CALL api_release_frame(
    KivoSubtitlePlaneHandle plane,
    const KivoSubtitleFrameLease* lease) KIVO_P8_NOEXCEPT;
KivoP8ApiStatus KIVO_P8_CALL api_acquire_snapshot(
    KivoSubtitlePlaneHandle plane,
    const KivoSubtitleSnapshotAcquireRequest* request,
    KivoSubtitleSnapshotLease* out_lease) KIVO_P8_NOEXCEPT;
KivoP8ApiStatus KIVO_P8_CALL api_copy_snapshot(
    KivoSubtitlePlaneHandle plane,
    const KivoSubtitleSnapshotLease* lease,
    KivoMutableByteSpan dst,
    uint64_t* out_required_bytes) KIVO_P8_NOEXCEPT;
KivoP8ApiStatus KIVO_P8_CALL api_release_snapshot(
    KivoSubtitlePlaneHandle plane,
    const KivoSubtitleSnapshotLease* lease) KIVO_P8_NOEXCEPT;
KivoP8ApiStatus KIVO_P8_CALL api_copy_text_buffer(
    KivoSubtitlePlaneHandle plane,
    KivoSubtitleResourceAccessToken token,
    KivoSubtitleTextBufferId text_buffer_id,
    KivoMutableByteSpan dst,
    uint64_t* out_required_bytes) KIVO_P8_NOEXCEPT;
KivoP8ApiStatus KIVO_P8_CALL api_copy_font_blob(
    KivoSubtitlePlaneHandle plane,
    KivoSubtitleResourceAccessToken token,
    KivoSubtitleFontBlobId font_blob_id,
    KivoMutableByteSpan dst,
    uint64_t* out_required_bytes) KIVO_P8_NOEXCEPT;
KivoP8ApiStatus KIVO_P8_CALL api_subscribe_events(
    KivoSubtitlePlaneHandle plane,
    const KivoSubtitleObserverCallbacks* callbacks,
    KivoSubtitleSubscriptionToken* out_token) KIVO_P8_NOEXCEPT;
KivoP8ApiStatus KIVO_P8_CALL api_unsubscribe_events(
    KivoSubtitlePlaneHandle plane,
    KivoSubtitleSubscriptionToken token) KIVO_P8_NOEXCEPT;
KivoP8ApiStatus KIVO_P8_CALL api_get_metrics(
    KivoSubtitlePlaneHandle plane,
    KivoSubtitleMetricsSnapshot* out_metrics) KIVO_P8_NOEXCEPT;
KivoP8ApiStatus KIVO_P8_CALL api_compute_redacted_source_identity(
    const KivoSubtitlePrivacyPolicy* privacy,
    const KivoSubtitleSourceDescriptor* source,
    KivoRedactedSourceIdentity* out_identity) KIVO_P8_NOEXCEPT;
KivoP8ApiStatus KIVO_P8_CALL api_classify_style_semantics(
    KivoSubtitleFormat format,
    KivoSubtitleInputStringView text,
    KivoSubtitleStyleSemanticsReport* out_report) KIVO_P8_NOEXCEPT;

}  // namespace kivo::video::subtitle_plane::runtime
