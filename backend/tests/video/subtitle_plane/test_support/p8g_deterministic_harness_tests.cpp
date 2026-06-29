#include "p8_fake_runtime.hpp"

namespace {

KivoSubtitlePlaneApiV1 api_table() {
    KivoSubtitlePlaneApiV1 api{};
    api.header.struct_size = sizeof(api);
    if (kivo_subtitle_plane_get_api_v1(KIVO_P8_ABI_REVISION, &api) != KivoP8ApiStatus_Ok) {
        api.header.struct_size = 0;
    }
    return api;
}

}  // namespace

int main() {
    KivoSubtitlePlaneApiV1 api = api_table();
    KivoSubtitlePlaneHandle plane{};
    if (api.create_plane(nullptr, &plane) != KivoP8ApiStatus_Ok) return 1;

    P8FakeReader reader{};
    KivoSubtitleSourceDescriptor source{};
    source.kind = KivoSubtitleSourceKind_Reader;
    source.reader.user_data = &reader;
    source.reader.open = p8_fake_open;
    source.reader.read = p8_fake_read;
    source.reader.close = p8_fake_close;
    KivoSubtitleCommandToken token{};
    if (api.set_source(plane, &source, KivoSubtitleCorrelationId{7}, &token) != KivoP8ApiStatus_Ok) return 2;
    if (reader.open_count != 1 || reader.read_count != 1 || reader.close_count != 1) return 3;

    P8FakeKey key{};
    KivoSubtitlePrivacyPolicy privacy{};
    privacy.hash_algorithm = KivoSubtitleHashAlgorithm_Blake2b_Keyed_64;
    privacy.hash_key_id = 5;
    privacy.key_provider.user_data = &key;
    privacy.key_provider.get_key = p8_fake_key;
    KivoRedactedSourceIdentity identity{};
    if (api.compute_redacted_source_identity(&privacy, &source, &identity) != KivoP8ApiStatus_Ok) return 4;
    if (key.call_count != 1 || identity.hash_key_id != 5 || identity.source_identity_hash == 0) return 5;

    KivoEmbeddedSubtitleStreamEndRequest stream_end{};
    stream_end.packet_generation.packet_generation = 1;
    KivoSubtitleCommandToken stream_token{};
    if (api.end_embedded_stream(plane, &stream_end, &stream_token) != KivoP8ApiStatus_Ok) return 6;
    if (stream_token.immediate_status != KivoSubtitleCommandImmediateStatus_Accepted) return 7;

    api.destroy_plane(plane);
    return 0;
}
