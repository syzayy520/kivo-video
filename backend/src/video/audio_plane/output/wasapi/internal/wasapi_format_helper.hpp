// wasapi_format_helper.hpp — P6D internal: WAVEFORMATEX construction/parsing
#pragma once

#include <cstdint>
#include <audioclient.h>
#include <mmreg.h>

namespace kivo::video::audio_plane::backend::output::wasapi::internal {

// Build a simple WAVEFORMATEX for PCM (16/24/32-bit, interleaved).
// Returns a WAVEFORMATEX by value (small enough for stack copy).
inline WAVEFORMATEX make_pcm_format(int32_t sample_rate, int32_t channels, int32_t bit_depth) noexcept {
    WAVEFORMATEX wfx{};
    wfx.wFormatTag = WAVE_FORMAT_PCM;
    wfx.nChannels = static_cast<WORD>(channels);
    wfx.nSamplesPerSec = static_cast<DWORD>(sample_rate);
    wfx.wBitsPerSample = static_cast<WORD>(bit_depth);
    wfx.nBlockAlign = static_cast<WORD>(channels * bit_depth / 8);
    wfx.nAvgBytesPerSec = static_cast<DWORD>(sample_rate) * wfx.nBlockAlign;
    wfx.cbSize = 0;
    return wfx;
}

// Validate that a WAVEFORMATEX is PCM and has consistent fields.
inline bool is_valid_pcm(const WAVEFORMATEX& wfx) noexcept {
    if (wfx.wFormatTag != WAVE_FORMAT_PCM) return false;
    if (wfx.nChannels == 0) return false;
    if (wfx.nSamplesPerSec == 0) return false;
    if (wfx.wBitsPerSample == 0) return false;
    if (wfx.nBlockAlign != wfx.nChannels * wfx.wBitsPerSample / 8) return false;
    if (wfx.nAvgBytesPerSec != wfx.nSamplesPerSec * wfx.nBlockAlign) return false;
    return true;
}

// Extract fields from WAVEFORMATEX into plain scalars.
struct PcmFormatInfo {
    int32_t sample_rate{0};
    int32_t channel_count{0};
    int32_t bit_depth{0};
};

inline PcmFormatInfo extract_pcm_info(const WAVEFORMATEX& wfx) noexcept {
    PcmFormatInfo info;
    info.sample_rate = static_cast<int32_t>(wfx.nSamplesPerSec);
    info.channel_count = static_cast<int32_t>(wfx.nChannels);
    info.bit_depth = static_cast<int32_t>(wfx.wBitsPerSample);
    return info;
}

}  // namespace kivo::video::audio_plane::backend::output::wasapi::internal
