#pragma once

#include <cstdint>
#include <string>

namespace kivo::p3::platform_contracts {

/// PlatformCapabilityHints provides platform facts as hints, not platform API implementation.
/// Windows desktop primary. Qt as UI tech stack note only.
/// D3D11/DXGI/WASAPI as hints/not_claimed.
/// schema_version: p3.platform.hints.v1
struct PlatformCapabilityHints
{
    std::string schema_version{"p3.platform.hints.v1"};
    std::string primary_platform{"windows_desktop"};
    std::string ui_tech_stack_note{"qt"};
    std::string video_api_hint{"d3d11"};
    std::string audio_api_hint{"wasapi"};
    bool has_hardware_decode{false};
    bool has_hdr_display{false};
    bool has_spdif{false};
    bool has_hdmi_audio{false};

    /// Hints must not trigger platform API calls.
    bool is_hint_only() const noexcept { return true; }
};

/// DecoderCapabilityRequirement describes decoder constraints for P4.
struct DecoderCapabilityRequirement
{
    std::string schema_version{"p3.platform.decoder_capability.v1"};
    std::string codec_hint{};
    bool hardware_decode_available{false};
    bool software_decode_available{true};
    int32_t max_resolution_width{0};
    int32_t max_resolution_height{0};
    int32_t max_fps{0};
};

/// AudioCapabilityRequirement describes audio constraints for P4.
struct AudioCapabilityRequirement
{
    std::string schema_version{"p3.platform.audio_capability.v1"};
    std::string audio_format_hint{};
    bool passthrough_available{false};
    bool pcm_output_available{true};
    int32_t max_channels{2};
    int32_t max_sample_rate{48000};
};

/// SubtitleCapabilityRequirement describes subtitle constraints for P4.
struct SubtitleCapabilityRequirement
{
    std::string schema_version{"p3.platform.subtitle_capability.v1"};
    bool bitmap_subtitle_supported{false};
    bool text_subtitle_supported{true};
    bool burn_in_supported{false};
};

/// BufferRequirement prepares P4 buffer requirements without executing buffer.
struct BufferRequirement
{
    std::string schema_version{"p3.platform.buffer_requirement.v1"};
    int64_t startup_prebuffer_bytes{0};
    int64_t low_watermark_bytes{0};
    int64_t high_watermark_bytes{0};
    int64_t max_bytes{0};
    bool dynamic_adjustment_enabled{false};
    bool low_power_mode{false};
    bool metered_network{false};
    bool cellular_network{false};
};

} // namespace kivo::p3::platform_contracts
