#ifndef KIVO_VIDEO_SUBTITLE_PLANE_PRIVACY_SUBTITLE_PRIVACY_H
#define KIVO_VIDEO_SUBTITLE_PLANE_PRIVACY_SUBTITLE_PRIVACY_H

#include "kivo/video/subtitle_plane/input/subtitle_input_views.h"
#include "kivo/video/subtitle_plane/types/subtitle_ids.h"

typedef enum KivoSubtitleHashAlgorithm {
    KivoSubtitleHashAlgorithm_None = 0,
    KivoSubtitleHashAlgorithm_SipHash24_64 = 1,
    KivoSubtitleHashAlgorithm_Blake2b_Keyed_64 = 2,
    KivoSubtitleHashAlgorithm_HmacSha256_Truncated64 = 3,
    KivoSubtitleHashAlgorithm_Blake2b_Unkeyed_TestOnly_64 = 4
} KivoSubtitleHashAlgorithm;

typedef struct KivoSubtitleHashKeyProvider {
    KivoP8AbiHeader header;
    void* user_data;
    KivoP8CallbackStatus (KIVO_P8_CALL *get_key)(
        void* user_data,
        uint64_t key_id,
        uint8_t* out_key,
        uint64_t out_key_capacity,
        uint64_t* out_key_bytes) KIVO_P8_NOEXCEPT;
} KivoSubtitleHashKeyProvider;

typedef enum KivoSubtitlePrivacyMode {
    KivoSubtitlePrivacyMode_Strict = 0,
    KivoSubtitlePrivacyMode_DiagnosticRedacted = 1,
    KivoSubtitlePrivacyMode_DiagnosticVerboseLocalOnly = 2
} KivoSubtitlePrivacyMode;

typedef struct KivoSubtitlePrivacyPolicy {
    KivoP8AbiHeader header;
    KivoSubtitlePrivacyMode mode;
    KivoSubtitleHashAlgorithm hash_algorithm;
    uint64_t hash_key_id;
    KivoSubtitleHashKeyProvider key_provider;
    uint32_t flags;
    uint32_t reserved;
} KivoSubtitlePrivacyPolicy;

typedef enum KivoRedactedSourceKind {
    KivoRedactedSourceKind_Unknown = 0,
    KivoRedactedSourceKind_LocalFile = 1,
    KivoRedactedSourceKind_NetworkUrl = 2,
    KivoRedactedSourceKind_ProviderStream = 3,
    KivoRedactedSourceKind_EmbeddedTrack = 4,
    KivoRedactedSourceKind_MemoryBuffer = 5
} KivoRedactedSourceKind;

typedef struct KivoRedactedSourceIdentity {
    KivoRedactedSourceKind kind;
    uint32_t flags;
    uint64_t source_identity_hash;
    uint64_t basename_hash;
    uint64_t content_fingerprint_hash;
    KivoSubtitleStringId extension;
    KivoSubtitleStringId scheme;
    uint64_t hash_key_id;
} KivoRedactedSourceIdentity;

#endif
