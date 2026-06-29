#include "kivo/video/subtitle_plane/kivo_subtitle_plane.h"

#include "video/subtitle_plane/discovery/discovery_priority.hpp"
#include "video/subtitle_plane/format/format_track_classification.hpp"

#include <cstddef>
#include <cstring>

namespace {

struct ReaderProbe {
    int open_count = 0;
    int read_count = 0;
    int cancel_count = 0;
    int close_count = 0;
};

struct KeyProbe {
    int calls = 0;
    uint8_t key_byte = 0xA5u;
    bool fail = false;
};

KivoP8CallbackStatus KIVO_P8_CALL get_key(void* user_data,
                                          uint64_t,
                                          uint8_t* out_key,
                                          uint64_t out_key_capacity,
                                          uint64_t* out_key_bytes) KIVO_P8_NOEXCEPT {
    auto* probe = static_cast<KeyProbe*>(user_data);
    probe->calls += 1;
    if (probe->fail || out_key == nullptr || out_key_capacity < 4 || out_key_bytes == nullptr) {
        return KivoP8CallbackStatus_Failed;
    }
    out_key[0] = probe->key_byte;
    out_key[1] = static_cast<uint8_t>(probe->key_byte + 1u);
    out_key[2] = static_cast<uint8_t>(probe->key_byte + 2u);
    out_key[3] = static_cast<uint8_t>(probe->key_byte + 3u);
    *out_key_bytes = 4;
    return KivoP8CallbackStatus_Ok;
}

KivoP8CallbackStatus KIVO_P8_CALL open_reader(void* user_data,
                                              const KivoSubtitleSourceDescriptor*,
                                              KivoSubtitleSourceHandle* out_handle) KIVO_P8_NOEXCEPT {
    auto* probe = static_cast<ReaderProbe*>(user_data);
    probe->open_count += 1;
    out_handle->value = 42;
    return KivoP8CallbackStatus_Ok;
}

KivoP8CallbackStatus KIVO_P8_CALL read_reader(void* user_data,
                                              KivoSubtitleSourceHandle,
                                              uint8_t*,
                                              uint64_t,
                                              uint64_t* out_read) KIVO_P8_NOEXCEPT {
    auto* probe = static_cast<ReaderProbe*>(user_data);
    probe->read_count += 1;
    *out_read = 0;
    return KivoP8CallbackStatus_Ok;
}

KivoP8CallbackStatus KIVO_P8_CALL fail_open(void* user_data,
                                            const KivoSubtitleSourceDescriptor*,
                                            KivoSubtitleSourceHandle*) KIVO_P8_NOEXCEPT {
    static_cast<ReaderProbe*>(user_data)->open_count += 1;
    return KivoP8CallbackStatus_Failed;
}

KivoP8CallbackStatus KIVO_P8_CALL fail_read(void* user_data,
                                            KivoSubtitleSourceHandle,
                                            uint8_t*,
                                            uint64_t,
                                            uint64_t*) KIVO_P8_NOEXCEPT {
    static_cast<ReaderProbe*>(user_data)->read_count += 1;
    return KivoP8CallbackStatus_Failed;
}

KivoP8CallbackStatus KIVO_P8_CALL cancel_reader(void* user_data,
                                                KivoSubtitleSourceHandle) KIVO_P8_NOEXCEPT {
    static_cast<ReaderProbe*>(user_data)->cancel_count += 1;
    return KivoP8CallbackStatus_Ok;
}

void KIVO_P8_CALL close_reader(void* user_data,
                               KivoSubtitleSourceHandle) KIVO_P8_NOEXCEPT {
    static_cast<ReaderProbe*>(user_data)->close_count += 1;
}

KivoSubtitlePlaneApiV1 api_table() {
    KivoSubtitlePlaneApiV1 api{};
    api.header.struct_size = sizeof(api);
    if (kivo_subtitle_plane_get_api_v1(KIVO_P8_ABI_REVISION, &api) != KivoP8ApiStatus_Ok) {
        api.header.struct_size = 0;
    }
    return api;
}

int expect_source_result(KivoSubtitlePlaneApiV1& api,
                         KivoSubtitlePlaneHandle plane,
                         const KivoSubtitleSourceDescriptor& source,
                         KivoP8ApiStatus expected_status,
                         KivoSubtitlePlaneError expected_error) {
    KivoSubtitleCommandToken token{};
    const KivoP8ApiStatus status =
        api.set_source(plane, &source, KivoSubtitleCorrelationId{31}, &token);
    if (status != expected_status) return 101;
    if (expected_error != KivoSubtitlePlaneError_None &&
        token.immediate_error != expected_error) {
        return 102;
    }
    return 0;
}

}  // namespace

int main() {
    KivoSubtitlePlaneApiV1 api = api_table();
    KivoSubtitlePlaneHandle plane{};
    if (api.create_plane(nullptr, &plane) != KivoP8ApiStatus_Ok) return 1;

    static_assert(offsetof(KivoSubtitleExternalSourceVTable, header) == 0);

    const uint8_t path[] = {'m', 'o', 'v', 'i', 'e', '.', 'e', 'n', '.', 's', 'r', 't'};
    KivoSubtitleSourceDescriptor file_source{};
    file_source.kind = KivoSubtitleSourceKind_FilePathUtf8;
    file_source.file_path_utf8.utf8 = path;
    file_source.file_path_utf8.utf8_bytes = sizeof(path);
    if (expect_source_result(api, plane, file_source, KivoP8ApiStatus_Ok,
                             KivoSubtitlePlaneError_None) != 0) return 11;

    KivoSubtitleSourceDescriptor bad_path{};
    bad_path.kind = KivoSubtitleSourceKind_FilePathUtf8;
    bad_path.file_path_utf8.utf8_bytes = 4;
    if (expect_source_result(api, plane, bad_path, KivoP8ApiStatus_InvalidArgument,
                             KivoSubtitlePlaneError_MalformedInput) != 0) return 12;

    const uint8_t bytes[] = {'1', '\n'};
    KivoSubtitleSourceDescriptor memory_source{};
    memory_source.kind = KivoSubtitleSourceKind_MemoryBytes;
    memory_source.memory_bytes.data = bytes;
    memory_source.memory_bytes.bytes = sizeof(bytes);
    if (expect_source_result(api, plane, memory_source, KivoP8ApiStatus_Ok,
                             KivoSubtitlePlaneError_None) != 0) return 13;

    KivoSubtitleSourceDescriptor bad_memory{};
    bad_memory.kind = KivoSubtitleSourceKind_MemoryBytes;
    bad_memory.memory_bytes.bytes = 2;
    if (expect_source_result(api, plane, bad_memory, KivoP8ApiStatus_InvalidArgument,
                             KivoSubtitlePlaneError_MalformedInput) != 0) return 14;

    KivoSubtitleSourceDescriptor embedded_source{};
    embedded_source.kind = KivoSubtitleSourceKind_EmbeddedTrack;
    if (expect_source_result(api, plane, embedded_source, KivoP8ApiStatus_Ok,
                             KivoSubtitlePlaneError_None) != 0) return 15;

    ReaderProbe probe{};
    KivoSubtitleSourceDescriptor source{};
    source.kind = KivoSubtitleSourceKind_Reader;
    source.source_id.value = 7;
    source.size_hint_bytes = 123;
    source.reader.user_data = &probe;
    source.reader.open = open_reader;
    source.reader.read = read_reader;
    source.reader.cancel = cancel_reader;
    source.reader.close = close_reader;

    KivoSubtitleCommandToken token{};
    if (api.set_source(plane, &source, KivoSubtitleCorrelationId{9}, &token) != KivoP8ApiStatus_Ok) return 2;
    if (probe.open_count != 1 || probe.read_count != 1 || probe.cancel_count != 1 || probe.close_count != 1) return 3;
    if (token.command_id.value == 0 || token.correlation_id.value != 9) return 4;

    ReaderProbe provider_probe{};
    KivoSubtitleSourceDescriptor provider{};
    provider.kind = KivoSubtitleSourceKind_ProviderByteStream;
    provider.reader = source.reader;
    provider.reader.user_data = &provider_probe;
    if (api.set_source(plane, &provider, KivoSubtitleCorrelationId{19}, &token) != KivoP8ApiStatus_Ok) return 16;
    if (provider_probe.open_count != 1 || provider_probe.read_count != 1 ||
        provider_probe.cancel_count != 1 || provider_probe.close_count != 1) {
        return 17;
    }

    ReaderProbe open_fail_probe{};
    KivoSubtitleSourceDescriptor open_fail_source = source;
    open_fail_source.reader.user_data = &open_fail_probe;
    open_fail_source.reader.open = fail_open;
    if (api.set_source(plane, &open_fail_source, KivoSubtitleCorrelationId{20}, &token) !=
        KivoP8ApiStatus_Failed) {
        return 18;
    }
    if (token.immediate_error != KivoSubtitlePlaneError_SubtitleSourceOpenFailed ||
        open_fail_probe.close_count != 0) {
        return 19;
    }

    ReaderProbe read_fail_probe{};
    KivoSubtitleSourceDescriptor read_fail_source = source;
    read_fail_source.reader.user_data = &read_fail_probe;
    read_fail_source.reader.read = fail_read;
    if (api.set_source(plane, &read_fail_source, KivoSubtitleCorrelationId{21}, &token) !=
        KivoP8ApiStatus_Failed) {
        return 20;
    }
    if (token.immediate_error != KivoSubtitlePlaneError_SubtitleSourceReadFailed ||
        read_fail_probe.close_count != 1) {
        return 21;
    }

    KivoSubtitlePrivacyPolicy privacy{};
    KeyProbe key_probe{};
    privacy.hash_key_id = 11;
    privacy.hash_algorithm = KivoSubtitleHashAlgorithm_HmacSha256_Truncated64;
    privacy.key_provider.user_data = &key_probe;
    privacy.key_provider.get_key = get_key;
    KivoRedactedSourceIdentity identity{};
    if (api.compute_redacted_source_identity(&privacy, &source, &identity) != KivoP8ApiStatus_Ok) return 5;
    if (identity.source_identity_hash == 0 || identity.hash_key_id != 11 || key_probe.calls != 1) return 6;

    KivoRedactedSourceIdentity identity_again{};
    if (api.compute_redacted_source_identity(&privacy, &source, &identity_again) != KivoP8ApiStatus_Ok) return 35;
    if (identity_again.source_identity_hash != identity.source_identity_hash) return 36;
    key_probe.key_byte = 0xB6u;
    KivoRedactedSourceIdentity identity_other_key{};
    if (api.compute_redacted_source_identity(&privacy, &source, &identity_other_key) != KivoP8ApiStatus_Ok) return 37;
    if (identity_other_key.source_identity_hash == identity.source_identity_hash) return 38;
    key_probe.fail = true;
    if (api.compute_redacted_source_identity(&privacy, &source, &identity_other_key) != KivoP8ApiStatus_Failed) {
        return 39;
    }

    KivoEmbeddedSubtitleStreamEndRequest stale{};
    stale.correlation_id.value = 10;
    KivoSubtitleCommandToken stream_token{};
    if (api.end_embedded_stream(plane, &stale, &stream_token) != KivoP8ApiStatus_Ok) return 7;
    if (stream_token.immediate_status != KivoSubtitleCommandImmediateStatus_NoOp) return 8;

    KivoEmbeddedSubtitleStreamEndRequest eos{};
    eos.correlation_id.value = 11;
    eos.packet_generation.packet_generation = 1;
    eos.flags = 1u;
    if (api.end_embedded_stream(plane, &eos, &stream_token) != KivoP8ApiStatus_Ok) return 22;
    if (stream_token.immediate_status != KivoSubtitleCommandImmediateStatus_Accepted) return 23;

    KivoEmbeddedSubtitleStreamEndRequest discontinuity{};
    discontinuity.correlation_id.value = 12;
    discontinuity.packet_generation.packet_generation = 2;
    discontinuity.flags = 2u;
    if (api.end_embedded_stream(plane, &discontinuity, &stream_token) != KivoP8ApiStatus_Ok) return 24;
    KivoSubtitleMetricsSnapshot metrics{};
    if (api.get_metrics(plane, &metrics) != KivoP8ApiStatus_Ok) return 25;
    if (metrics.generation_discard_count != 1 || metrics.packet_drop_count != 1) return 26;

    KivoSubtitleResourceAccessToken bad_frame{};
    bad_frame.kind = KivoSubtitleResourceAccessKind_FrameLease;
    bad_frame.lease_id = 1;
    bad_frame.lease_generation = 1;
    bad_frame.snapshot_lease_id.value = 99;
    bad_frame.snapshot_revision.value = 1;
    uint64_t required = 0;
    if (api.copy_text_buffer(plane, bad_frame, KivoSubtitleTextBufferId{1}, KivoMutableByteSpan{}, &required) !=
        KivoP8ApiStatus_InvalidArgument) {
        return 9;
    }

    KivoRedactedSourceIdentity external_identity{};
    external_identity.source_identity_hash = 3;
    const uint64_t external_first =
        kivo::video::subtitle_plane::runtime::discovery_priority_key(1u << 4u, 1u, 5u, 2u, external_identity);
    const uint64_t embedded_second =
        kivo::video::subtitle_plane::runtime::discovery_priority_key(1u << 4u, 0u, 5u, 2u, external_identity);
    if (!(external_first < embedded_second)) return 10;

    using kivo::video::subtitle_plane::runtime::subtitle_format_for_embedded_codec;
    using kivo::video::subtitle_plane::runtime::subtitle_format_is_bitmap;
    using kivo::video::subtitle_plane::runtime::subtitle_format_is_text;
    using kivo::video::subtitle_plane::runtime::subtitle_track_kind_for_source;
    if (!subtitle_format_is_text(KivoSubtitleFormat_Srt) ||
        !subtitle_format_is_text(KivoSubtitleFormat_Ass) ||
        !subtitle_format_is_text(KivoSubtitleFormat_Ssa) ||
        !subtitle_format_is_text(KivoSubtitleFormat_WebVtt)) {
        return 27;
    }
    if (!subtitle_format_is_bitmap(KivoSubtitleFormat_Pgs) ||
        !subtitle_format_is_bitmap(KivoSubtitleFormat_VobSub) ||
        !subtitle_format_is_bitmap(KivoSubtitleFormat_DvbSub)) {
        return 28;
    }
    if (subtitle_format_for_embedded_codec(KivoSubtitleEmbeddedCodec_MatroskaAssDialogue) !=
        KivoSubtitleFormat_Ass) {
        return 29;
    }
    if (subtitle_format_for_embedded_codec(KivoSubtitleEmbeddedCodec_Mp4WebVttSample) !=
        KivoSubtitleFormat_WebVtt) {
        return 30;
    }
    if (subtitle_format_for_embedded_codec(KivoSubtitleEmbeddedCodec_MovText) !=
        KivoSubtitleFormat_Unsupported) {
        return 31;
    }
    if (subtitle_track_kind_for_source(KivoSubtitleSourceKind_FilePathUtf8, KivoSubtitleFormat_Srt) !=
        KivoSubtitleTrackKind_ExternalFile) {
        return 32;
    }
    if (subtitle_track_kind_for_source(KivoSubtitleSourceKind_EmbeddedTrack, KivoSubtitleFormat_WebVtt) !=
        KivoSubtitleTrackKind_EmbeddedPacket) {
        return 33;
    }
    if (subtitle_track_kind_for_source(KivoSubtitleSourceKind_EmbeddedTrack, KivoSubtitleFormat_Pgs) !=
        KivoSubtitleTrackKind_UnsupportedBitmap) {
        return 34;
    }

    api.destroy_plane(plane);
    return 0;
}
