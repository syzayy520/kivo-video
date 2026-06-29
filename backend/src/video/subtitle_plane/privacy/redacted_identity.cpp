#include "video/subtitle_plane/privacy/redacted_identity.hpp"

namespace kivo::video::subtitle_plane::runtime {
namespace {

uint64_t mix64(uint64_t value) noexcept {
    value ^= value >> 33u;
    value *= 0xff51afd7ed558ccdull;
    value ^= value >> 33u;
    value *= 0xc4ceb9fe1a85ec53ull;
    value ^= value >> 33u;
    return value == 0 ? 1 : value;
}

uint64_t hash_bytes(uint64_t seed, const uint8_t* const data, const uint64_t bytes) noexcept {
    uint64_t value = seed ^ 0xcbf29ce484222325ull;
    for (uint64_t index = 0; index < bytes; ++index) {
        value ^= static_cast<uint64_t>(data[index]);
        value *= 0x100000001b3ull;
    }
    return mix64(value ^ bytes);
}

bool load_hash_seed(const KivoSubtitlePrivacyPolicy* const privacy,
                    uint64_t* const out_seed) noexcept {
    if (privacy == nullptr ||
        privacy->hash_algorithm == KivoSubtitleHashAlgorithm_None ||
        privacy->hash_algorithm == KivoSubtitleHashAlgorithm_Blake2b_Unkeyed_TestOnly_64) {
        *out_seed = mix64(privacy == nullptr ? 0 : privacy->hash_key_id);
        return true;
    }
    if (privacy->key_provider.get_key == nullptr) {
        return false;
    }
    uint8_t key[64]{};
    uint64_t key_bytes = 0;
    const KivoP8CallbackStatus status = privacy->key_provider.get_key(
        privacy->key_provider.user_data,
        privacy->hash_key_id,
        key,
        sizeof(key),
        &key_bytes);
    if (status != KivoP8CallbackStatus_Ok || key_bytes == 0 || key_bytes > sizeof(key)) {
        return false;
    }
    *out_seed = hash_bytes(mix64(privacy->hash_key_id), key, key_bytes);
    return true;
}

KivoRedactedSourceKind redacted_kind_for_source(const KivoSubtitleSourceKind kind) noexcept {
    if (kind == KivoSubtitleSourceKind_FilePathUtf8) return KivoRedactedSourceKind_LocalFile;
    if (kind == KivoSubtitleSourceKind_EmbeddedTrack) return KivoRedactedSourceKind_EmbeddedTrack;
    if (kind == KivoSubtitleSourceKind_MemoryBytes) return KivoRedactedSourceKind_MemoryBuffer;
    if (kind == KivoSubtitleSourceKind_Reader ||
        kind == KivoSubtitleSourceKind_ProviderByteStream) {
        return KivoRedactedSourceKind_ProviderStream;
    }
    return KivoRedactedSourceKind_Unknown;
}

KivoSubtitleInputStringView basename_view(const KivoSubtitleInputStringView path) noexcept {
    uint64_t begin = 0;
    for (uint64_t index = 0; index < path.utf8_bytes; ++index) {
        if (path.utf8[index] == static_cast<uint8_t>('/') ||
            path.utf8[index] == static_cast<uint8_t>('\\')) {
            begin = index + 1;
        }
    }
    KivoSubtitleInputStringView view{};
    view.utf8 = path.utf8 == nullptr ? nullptr : path.utf8 + begin;
    view.utf8_bytes = path.utf8_bytes - begin;
    return view;
}

}  // namespace

KivoP8ApiStatus compute_redacted_identity(
    const KivoSubtitlePrivacyPolicy* const privacy,
    const KivoSubtitleSourceDescriptor& source,
    KivoRedactedSourceIdentity* const out_identity) noexcept {
    if (out_identity == nullptr) {
        return KivoP8ApiStatus_NullPointer;
    }
    if (source.kind == KivoSubtitleSourceKind_Unknown) {
        return KivoP8ApiStatus_InvalidArgument;
    }

    uint64_t hash_seed = 0;
    if (!load_hash_seed(privacy, &hash_seed)) {
        return KivoP8ApiStatus_Failed;
    }

    KivoRedactedSourceIdentity identity = source.redacted_identity;
    if (identity.kind == KivoRedactedSourceKind_Unknown) {
        identity.kind = redacted_kind_for_source(source.kind);
    }
    const uint64_t key_id = privacy != nullptr ? privacy->hash_key_id : 0;
    identity.hash_key_id = key_id;
    identity.flags |= privacy == nullptr ? 0x1u : 0x3u;
    identity.source_identity_hash = mix64(
        hash_seed ^
        source.source_id.value ^
        (static_cast<uint64_t>(source.kind) << 48u) ^
        source.size_hint_bytes);
    if (source.kind == KivoSubtitleSourceKind_FilePathUtf8 &&
        source.file_path_utf8.utf8 != nullptr) {
        const KivoSubtitleInputStringView basename = basename_view(source.file_path_utf8);
        identity.basename_hash =
            hash_bytes(hash_seed ^ 0xB45Eull, basename.utf8, basename.utf8_bytes);
        identity.source_identity_hash =
            hash_bytes(hash_seed ^ 0x51Dull, source.file_path_utf8.utf8,
                       source.file_path_utf8.utf8_bytes);
    } else {
        identity.basename_hash = mix64(identity.source_identity_hash ^ 0xB45Eull);
    }
    if (source.kind == KivoSubtitleSourceKind_MemoryBytes &&
        source.memory_bytes.data != nullptr) {
        identity.content_fingerprint_hash =
            hash_bytes(hash_seed ^ 0xC0FFEEull, source.memory_bytes.data, source.memory_bytes.bytes);
    } else {
        identity.content_fingerprint_hash = mix64(identity.source_identity_hash ^ 0xC0FFEEull);
    }
    *out_identity = identity;
    return KivoP8ApiStatus_Ok;
}

}  // namespace kivo::video::subtitle_plane::runtime
