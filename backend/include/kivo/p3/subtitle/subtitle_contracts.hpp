#pragma once

#include <cstdint>
#include <optional>
#include <string>

namespace kivo::p3::subtitle {

/// SubtitleDownloadBoundary defines subtitle download boundary (no rendering).
/// schema_version: p3.subtitle.boundary.v1
struct SubtitleDownloadBoundary
{
    std::string schema_version{"p3.subtitle.boundary.v1"};
    bool download_supported{false};
    bool external_subtitle_supported{true};
    int64_t max_download_bytes{1048576};
    std::string redacted_source_url{};
};

/// SubtitleStyleContract defines subtitle style without rendering.
struct SubtitleStyleContract
{
    std::string schema_version{"p3.subtitle.style.v1"};
    std::string font_family{};
    int32_t font_size{0};
    std::string color{};
    std::string background_color{};
    bool bold{false};
    bool italic{false};
};

/// SubtitleSyncContract defines subtitle sync without rendering.
struct SubtitleSyncContract
{
    std::string schema_version{"p3.subtitle.sync.v1"};
    int64_t offset_ns{0};
    double speed_multiplier{1.0};
};

/// SubtitleBurnInDecision defines burn-in decision without rendering.
struct SubtitleBurnInDecision
{
    std::string schema_version{"p3.subtitle.burn_in.v1"};
    bool burn_in_required{false};
    bool server_processing_required{false};
    std::string reason{};

    bool can_express_server_processing() const noexcept { return server_processing_required; }
};

} // namespace kivo::p3::subtitle
