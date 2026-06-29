#include <cstddef>
#include <type_traits>

#include "kivo/video/subtitle_plane/kivo_subtitle_plane.h"

static_assert(KIVO_P8_ABI_MAJOR == 1u);
static_assert(KIVO_P8_ABI_MINOR == 5u);
static_assert(KIVO_P8_ABI_PATCH == 5u);
static_assert(KIVO_P8_ABI_REVISION == 155u);

static_assert(std::is_trivially_copyable_v<KivoP8AbiHeader>);
static_assert(std::is_trivially_copyable_v<KivoSubtitleCommandToken>);
static_assert(std::is_trivially_copyable_v<KivoSubtitleFrameLease>);
static_assert(std::is_trivially_copyable_v<KivoSubtitleSnapshotLease>);
static_assert(std::is_trivially_copyable_v<KivoSubtitleObserverCallbacks>);
static_assert(std::is_trivially_copyable_v<KivoEmbeddedSubtitleStreamEndRequest>);
static_assert(std::is_trivially_copyable_v<KivoMutableByteSpan>);
static_assert(std::is_trivially_copyable_v<KivoSubtitleInputBytesView>);

static_assert(offsetof(KivoSubtitlePlaneApiV1, header) == 0u);
static_assert(offsetof(KivoSubtitleExternalSourceVTable, header) == 0u);
static_assert(offsetof(KivoSubtitleObserverCallbacks, header) == 0u);
static_assert(offsetof(KivoEmbeddedSubtitleStreamEndRequest, header) == 0u);

static_assert(sizeof(KivoP8AbiHeader) % 8u == 0u);
static_assert(sizeof(KivoSubtitlePlaneApiV1) % 8u == 0u);
static_assert(sizeof(KivoSubtitleSourceDescriptor) % 8u == 0u);
static_assert(sizeof(KivoSubtitleSnapshotBlobHeader) % 8u == 0u);

#define KIVO_P8_ASSERT_ABI_STRUCT(TypeName) \
    static_assert(sizeof(TypeName) % 8u == 0u); \
    static_assert(alignof(TypeName) >= 8u)

KIVO_P8_ASSERT_ABI_STRUCT(KivoP8AbiHeader);
KIVO_P8_ASSERT_ABI_STRUCT(KivoSubtitlePlaneHandle);
KIVO_P8_ASSERT_ABI_STRUCT(KivoSubtitleCueId);
KIVO_P8_ASSERT_ABI_STRUCT(KivoSubtitlePacketId);
KIVO_P8_ASSERT_ABI_STRUCT(KivoSubtitleHashKeyProvider);
KIVO_P8_ASSERT_ABI_STRUCT(KivoSubtitlePrivacyPolicy);
KIVO_P8_ASSERT_ABI_STRUCT(KivoRedactedSourceIdentity);
KIVO_P8_ASSERT_ABI_STRUCT(KivoSubtitleSourceHandle);
KIVO_P8_ASSERT_ABI_STRUCT(KivoSubtitleExternalSourceVTable);
KIVO_P8_ASSERT_ABI_STRUCT(KivoSubtitleSourceDescriptor);
KIVO_P8_ASSERT_ABI_STRUCT(KivoSubtitleInputStringView);
KIVO_P8_ASSERT_ABI_STRUCT(KivoSubtitleInputBytesView);
KIVO_P8_ASSERT_ABI_STRUCT(KivoMutableByteSpan);
KIVO_P8_ASSERT_ABI_STRUCT(KivoSubtitleContentGeneration);
KIVO_P8_ASSERT_ABI_STRUCT(KivoSubtitlePacketGeneration);
KIVO_P8_ASSERT_ABI_STRUCT(KivoSubtitleTimelineGeneration);
KIVO_P8_ASSERT_ABI_STRUCT(KivoSubtitleFrameGeneration);
KIVO_P8_ASSERT_ABI_STRUCT(KivoSubtitleCommandToken);
KIVO_P8_ASSERT_ABI_STRUCT(KivoSubtitleCommandLifecycleSnapshot);
KIVO_P8_ASSERT_ABI_STRUCT(KivoSubtitleFrameLease);
KIVO_P8_ASSERT_ABI_STRUCT(KivoSubtitleSnapshotLease);
KIVO_P8_ASSERT_ABI_STRUCT(KivoSubtitleSnapshotBlobHeader);
KIVO_P8_ASSERT_ABI_STRUCT(KivoSubtitleSnapshotBlobSection);
KIVO_P8_ASSERT_ABI_STRUCT(KivoSubtitleEvidenceSummary);
KIVO_P8_ASSERT_ABI_STRUCT(KivoSubtitleMetricsSnapshot);
KIVO_P8_ASSERT_ABI_STRUCT(KivoSubtitleResourceAccessToken);
KIVO_P8_ASSERT_ABI_STRUCT(KivoSubtitleCancellationView);
KIVO_P8_ASSERT_ABI_STRUCT(KivoSubtitleEvent);
KIVO_P8_ASSERT_ABI_STRUCT(KivoSubtitleObserverCallbacks);
KIVO_P8_ASSERT_ABI_STRUCT(KivoEmbeddedSubtitleStreamEndRequest);
KIVO_P8_ASSERT_ABI_STRUCT(KivoSubtitlePlaneRuntimeCaps);
KIVO_P8_ASSERT_ABI_STRUCT(KivoSubtitlePlaneConfig);
KIVO_P8_ASSERT_ABI_STRUCT(KivoSubtitleDiscoveryBeginRequest);
KIVO_P8_ASSERT_ABI_STRUCT(KivoSubtitleFrameBuildRequest);
KIVO_P8_ASSERT_ABI_STRUCT(KivoSubtitleSnapshotAcquireRequest);
KIVO_P8_ASSERT_ABI_STRUCT(KivoSubtitleStyleSemanticsReport);
KIVO_P8_ASSERT_ABI_STRUCT(KivoSubtitlePlaneApiV1);

int main() {
    KivoSubtitleCommandToken rejected{};
    if (rejected.command_id.value != 0u) {
        return 1;
    }

    KivoSubtitleResourceAccessToken frame_token{};
    frame_token.kind = KivoSubtitleResourceAccessKind_FrameLease;
    frame_token.lease_id = 1;
    frame_token.lease_generation = 1;
    frame_token.snapshot_revision.value = 1;
    if (frame_token.snapshot_lease_id.value != 0u) {
        return 2;
    }

    KivoSubtitleInputBytesView bytes{};
    KivoMutableByteSpan span{};
    return bytes.bytes == 0u && span.bytes == 0u ? 0 : 3;
}
