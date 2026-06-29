#ifndef KIVO_VIDEO_SUBTITLE_PLANE_SOURCE_SUBTITLE_SOURCE_H
#define KIVO_VIDEO_SUBTITLE_PLANE_SOURCE_SUBTITLE_SOURCE_H

#include "kivo/video/subtitle_plane/privacy/subtitle_privacy.h"

typedef struct KivoSubtitleSourceHandle { uint64_t value; } KivoSubtitleSourceHandle;

typedef enum KivoSubtitleSeekOrigin {
    KivoSubtitleSeekOrigin_Begin = 0,
    KivoSubtitleSeekOrigin_Current = 1,
    KivoSubtitleSeekOrigin_End = 2
} KivoSubtitleSeekOrigin;

typedef enum KivoSubtitleSourceKind {
    KivoSubtitleSourceKind_Unknown = 0,
    KivoSubtitleSourceKind_FilePathUtf8 = 1,
    KivoSubtitleSourceKind_MemoryBytes = 2,
    KivoSubtitleSourceKind_Reader = 3,
    KivoSubtitleSourceKind_ProviderByteStream = 4,
    KivoSubtitleSourceKind_EmbeddedTrack = 5
} KivoSubtitleSourceKind;

typedef struct KivoSubtitleSourceDescriptor KivoSubtitleSourceDescriptor;

typedef struct KivoSubtitleExternalSourceVTable {
    KivoP8AbiHeader header;
    void* user_data;
    KivoP8CallbackStatus (KIVO_P8_CALL *open)(
        void* user_data,
        const KivoSubtitleSourceDescriptor* source,
        KivoSubtitleSourceHandle* out_handle) KIVO_P8_NOEXCEPT;
    KivoP8CallbackStatus (KIVO_P8_CALL *read)(
        void* user_data,
        KivoSubtitleSourceHandle handle,
        uint8_t* dst,
        uint64_t capacity,
        uint64_t* out_read) KIVO_P8_NOEXCEPT;
    KivoP8CallbackStatus (KIVO_P8_CALL *seek)(
        void* user_data,
        KivoSubtitleSourceHandle handle,
        int64_t offset,
        KivoSubtitleSeekOrigin origin) KIVO_P8_NOEXCEPT;
    KivoP8CallbackStatus (KIVO_P8_CALL *tell)(
        void* user_data,
        KivoSubtitleSourceHandle handle,
        uint64_t* out_position) KIVO_P8_NOEXCEPT;
    KivoP8CallbackStatus (KIVO_P8_CALL *cancel)(
        void* user_data,
        KivoSubtitleSourceHandle handle) KIVO_P8_NOEXCEPT;
    void (KIVO_P8_CALL *close)(
        void* user_data,
        KivoSubtitleSourceHandle handle) KIVO_P8_NOEXCEPT;
} KivoSubtitleExternalSourceVTable;

struct KivoSubtitleSourceDescriptor {
    KivoP8AbiHeader header;
    KivoSubtitleSourceKind kind;
    uint32_t flags;
    KivoSubtitleSourceId source_id;
    KivoSubtitleInputStringView file_path_utf8;
    KivoSubtitleInputBytesView memory_bytes;
    KivoSubtitleExternalSourceVTable reader;
    KivoRedactedSourceIdentity redacted_identity;
    uint64_t size_hint_bytes;
    uint64_t reserved0;
};

#endif
