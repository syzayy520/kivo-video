#ifndef KIVO_VIDEO_SUBTITLE_PLANE_API_SUBTITLE_PLANE_API_V1_H
#define KIVO_VIDEO_SUBTITLE_PLANE_API_SUBTITLE_PLANE_API_V1_H

#include "kivo/video/subtitle_plane/command/subtitle_command.h"
#include "kivo/video/subtitle_plane/observer/subtitle_observer.h"
#include "kivo/video/subtitle_plane/request/subtitle_plane_requests.h"
#include "kivo/video/subtitle_plane/resource/subtitle_resource_token.h"
#include "kivo/video/subtitle_plane/stream/subtitle_stream_end.h"

KIVO_P8_EXTERN_C_BEGIN

typedef struct KivoSubtitlePlaneApiV1 {
    KivoP8AbiHeader header;
    KivoP8ApiStatus (KIVO_P8_CALL *create_plane)(
        const KivoSubtitlePlaneConfig* config,
        KivoSubtitlePlaneHandle* out_plane) KIVO_P8_NOEXCEPT;
    KivoP8ApiStatus (KIVO_P8_CALL *close_plane)(
        KivoSubtitlePlaneHandle plane) KIVO_P8_NOEXCEPT;
    KivoP8ApiStatus (KIVO_P8_CALL *destroy_plane)(
        KivoSubtitlePlaneHandle plane) KIVO_P8_NOEXCEPT;
    KivoP8ApiStatus (KIVO_P8_CALL *begin_discovery)(
        KivoSubtitlePlaneHandle plane,
        const KivoSubtitleDiscoveryBeginRequest* request,
        KivoSubtitleCommandToken* out_token) KIVO_P8_NOEXCEPT;
    KivoP8ApiStatus (KIVO_P8_CALL *set_source)(
        KivoSubtitlePlaneHandle plane,
        const KivoSubtitleSourceDescriptor* source,
        KivoSubtitleCorrelationId correlation_id,
        KivoSubtitleCommandToken* out_token) KIVO_P8_NOEXCEPT;
    KivoP8ApiStatus (KIVO_P8_CALL *end_embedded_stream)(
        KivoSubtitlePlaneHandle plane,
        const KivoEmbeddedSubtitleStreamEndRequest* request,
        KivoSubtitleCommandToken* out_token) KIVO_P8_NOEXCEPT;
    KivoP8ApiStatus (KIVO_P8_CALL *query_command)(
        KivoSubtitlePlaneHandle plane,
        KivoSubtitleCommandId command_id,
        KivoSubtitleCommandLifecycleSnapshot* out_snapshot) KIVO_P8_NOEXCEPT;
    KivoP8ApiStatus (KIVO_P8_CALL *build_frame)(
        KivoSubtitlePlaneHandle plane,
        const KivoSubtitleFrameBuildRequest* request,
        KivoSubtitleFrameLease* out_lease) KIVO_P8_NOEXCEPT;
    KivoP8ApiStatus (KIVO_P8_CALL *release_frame)(
        KivoSubtitlePlaneHandle plane,
        const KivoSubtitleFrameLease* lease) KIVO_P8_NOEXCEPT;
    KivoP8ApiStatus (KIVO_P8_CALL *acquire_snapshot)(
        KivoSubtitlePlaneHandle plane,
        const KivoSubtitleSnapshotAcquireRequest* request,
        KivoSubtitleSnapshotLease* out_lease) KIVO_P8_NOEXCEPT;
    KivoP8ApiStatus (KIVO_P8_CALL *copy_snapshot)(
        KivoSubtitlePlaneHandle plane,
        const KivoSubtitleSnapshotLease* lease,
        KivoMutableByteSpan dst,
        uint64_t* out_required_bytes) KIVO_P8_NOEXCEPT;
    KivoP8ApiStatus (KIVO_P8_CALL *release_snapshot)(
        KivoSubtitlePlaneHandle plane,
        const KivoSubtitleSnapshotLease* lease) KIVO_P8_NOEXCEPT;
    KivoP8ApiStatus (KIVO_P8_CALL *copy_text_buffer)(
        KivoSubtitlePlaneHandle plane,
        KivoSubtitleResourceAccessToken token,
        KivoSubtitleTextBufferId text_buffer_id,
        KivoMutableByteSpan dst,
        uint64_t* out_required_bytes) KIVO_P8_NOEXCEPT;
    KivoP8ApiStatus (KIVO_P8_CALL *copy_font_blob)(
        KivoSubtitlePlaneHandle plane,
        KivoSubtitleResourceAccessToken token,
        KivoSubtitleFontBlobId font_blob_id,
        KivoMutableByteSpan dst,
        uint64_t* out_required_bytes) KIVO_P8_NOEXCEPT;
    KivoP8ApiStatus (KIVO_P8_CALL *subscribe_events)(
        KivoSubtitlePlaneHandle plane,
        const KivoSubtitleObserverCallbacks* callbacks,
        KivoSubtitleSubscriptionToken* out_token) KIVO_P8_NOEXCEPT;
    KivoP8ApiStatus (KIVO_P8_CALL *unsubscribe_events)(
        KivoSubtitlePlaneHandle plane,
        KivoSubtitleSubscriptionToken token) KIVO_P8_NOEXCEPT;
    KivoP8ApiStatus (KIVO_P8_CALL *get_metrics)(
        KivoSubtitlePlaneHandle plane,
        KivoSubtitleMetricsSnapshot* out_metrics) KIVO_P8_NOEXCEPT;
    KivoP8ApiStatus (KIVO_P8_CALL *compute_redacted_source_identity)(
        const KivoSubtitlePrivacyPolicy* privacy,
        const KivoSubtitleSourceDescriptor* source,
        KivoRedactedSourceIdentity* out_identity) KIVO_P8_NOEXCEPT;
    KivoP8ApiStatus (KIVO_P8_CALL *classify_style_semantics)(
        KivoSubtitleFormat format,
        KivoSubtitleInputStringView text,
        KivoSubtitleStyleSemanticsReport* out_report) KIVO_P8_NOEXCEPT;
    void* reserved_tail[4];
} KivoSubtitlePlaneApiV1;

#define KIVO_P8_API_V1_REQUIRED_PREFIX_SIZE ((uint64_t)offsetof(KivoSubtitlePlaneApiV1, reserved_tail))

KIVO_P8_API KivoP8ApiStatus KIVO_P8_CALL kivo_subtitle_plane_get_api_v1(
    uint32_t requested_abi_revision,
    KivoSubtitlePlaneApiV1* out_api) KIVO_P8_NOEXCEPT;

KIVO_P8_EXTERN_C_END

#endif
