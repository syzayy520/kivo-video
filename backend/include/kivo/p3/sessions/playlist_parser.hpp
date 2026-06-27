#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace kivo::p3::sessions {

/// PlaylistParsingResult defines playlist parsing strategy result.
/// #EXTM3U is necessary but not sufficient for HLS.
/// HLS tags route to HLS parsing. Simple URLs/EXTINF without HLS tags route to simple playlist.
/// schema_version: p3.session.playlist_parsing.v1
struct PlaylistParsingResult
{
    std::string schema_version{"p3.session.playlist_parsing.v1"};
    bool has_extm3u{false};
    bool has_hls_tags{false};
    bool is_hls{false};
    bool is_simple_playlist{false};
    std::string ambiguous_policy{"prefer_hls_if_hls_tags_present"};
    bool is_deterministic{true};

    bool is_valid() const noexcept { return is_hls != is_simple_playlist; }
};

/// PlaylistEntry defines a resolved playlist entry.
struct PlaylistEntry
{
    std::string entry_id{};
    std::string redacted_url{};
    std::string title{};
    int64_t duration_ns{0};
    bool is_nested_playlist{false};
};

/// PlaylistRecursionResult defines recursion policy result.
struct PlaylistRecursionResult
{
    std::string schema_version{"p3.session.playlist_recursion.v1"};
    int32_t max_depth{5};
    int32_t max_entries{1000};
    bool allow_nested_playlist{true};
    bool loop_detected{false};
    int32_t entries_resolved{0};
    std::vector<std::string> visited_redacted_ids{};
    std::string failure_reason{};

    bool is_within_limits() const noexcept
    {
        return !loop_detected && entries_resolved <= max_entries;
    }
};

/// Classify playlist content: simple vs HLS.
inline PlaylistParsingResult classify_playlist(const std::string& content)
{
    PlaylistParsingResult result;
    result.has_extm3u = content.find("#EXTM3U") != std::string::npos;
    result.has_hls_tags = content.find("#EXT-X-") != std::string::npos;

    if (!result.has_extm3u)
    {
        // No EXTM3U: could be PLS or unknown, treat as simple
        result.is_simple_playlist = true;
        result.is_hls = false;
    }
    else if (result.has_hls_tags)
    {
        // EXTM3U + HLS tags → HLS
        result.is_hls = true;
        result.is_simple_playlist = false;
    }
    else
    {
        // EXTM3U but no HLS tags → simple playlist
        result.is_simple_playlist = true;
        result.is_hls = false;
    }
    return result;
}

} // namespace kivo::p3::sessions
