#pragma once

#include "kivo/video/subtitle_plane/kivo_subtitle_plane.h"

struct P8FakeReader {
    int open_count = 0;
    int read_count = 0;
    int close_count = 0;
};

inline KivoP8CallbackStatus KIVO_P8_CALL p8_fake_open(
    void* user_data,
    const KivoSubtitleSourceDescriptor*,
    KivoSubtitleSourceHandle* out_handle) KIVO_P8_NOEXCEPT {
    auto* reader = static_cast<P8FakeReader*>(user_data);
    reader->open_count += 1;
    out_handle->value = 1;
    return KivoP8CallbackStatus_Ok;
}

inline KivoP8CallbackStatus KIVO_P8_CALL p8_fake_read(
    void* user_data,
    KivoSubtitleSourceHandle,
    uint8_t*,
    uint64_t,
    uint64_t* out_read) KIVO_P8_NOEXCEPT {
    auto* reader = static_cast<P8FakeReader*>(user_data);
    reader->read_count += 1;
    *out_read = 0;
    return KivoP8CallbackStatus_Ok;
}

inline void KIVO_P8_CALL p8_fake_close(
    void* user_data,
    KivoSubtitleSourceHandle) KIVO_P8_NOEXCEPT {
    static_cast<P8FakeReader*>(user_data)->close_count += 1;
}

struct P8FakeKey {
    int call_count = 0;
};

inline KivoP8CallbackStatus KIVO_P8_CALL p8_fake_key(
    void* user_data,
    uint64_t,
    uint8_t* out_key,
    uint64_t out_key_capacity,
    uint64_t* out_key_bytes) KIVO_P8_NOEXCEPT {
    static_cast<P8FakeKey*>(user_data)->call_count += 1;
    if (out_key_capacity < 2) return KivoP8CallbackStatus_Failed;
    out_key[0] = 0x4b;
    out_key[1] = 0x38;
    *out_key_bytes = 2;
    return KivoP8CallbackStatus_Ok;
}
