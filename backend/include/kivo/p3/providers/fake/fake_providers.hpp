#pragma once

#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>
#include "kivo/p3/source_model/source_kinds.hpp"
#include "kivo/p3/source_model/source_identity.hpp"
#include "kivo/p3/sessions/sessions.hpp"

namespace kivo::p3::providers::fake {

/// FakeProviderKind enumerates fake provider types.
enum class FakeProviderKind
{
    http,
    webdav,
    emby,
    jellyfin,
    alist,
    hls,
    dash,
    playlist
};

/// FakeProviderResponse simulates a provider response with deterministic data.
struct FakeProviderResponse
{
    std::string schema_version{"p3.fake.provider.response.v1"};
    bool success{false};
    std::vector<uint8_t> data{};
    int64_t content_length{0};
    int32_t status_code{200};
    std::string redacted_url{};
    std::string error_reason{};
    bool is_cancelled{false};
};

/// FakeProviderConfig configures a fake provider for deterministic testing.
struct FakeProviderConfig
{
    std::string schema_version{"p3.fake.provider.config.v1"};
    FakeProviderKind kind{FakeProviderKind::http};
    std::string provider_id{};
    std::string redacted_host{};
    int32_t port{0};
    bool supports_range_read{true};
    bool supports_seek{true};
    bool auth_required{false};
    std::string redacted_credential_scope{};
    int64_t simulated_content_length{1024};
    int64_t simulated_latency_ns{0};
    bool simulate_failure{false};
    bool simulate_timeout{false};
    bool simulate_cancel{false};
};

/// FakeProvider simulates a provider with deterministic behavior.
/// No real network. No real accounts. No real tokens.
class FakeProvider
{
public:
    explicit FakeProvider(FakeProviderConfig config);

    /// Open session with plan.
    bool open_session(const kivo::p3::sessions::SourceSessionPlan& plan);

    /// Read bytes at offset.
    FakeProviderResponse read(int64_t offset, int64_t length);

    /// Seek to offset.
    bool seek(int64_t offset);

    /// Close session.
    bool close_session();

    /// Get provider identity.
    const kivo::p3::source_model::ProviderIdentity& identity() const noexcept { return identity_; }

    /// Get current session state.
    bool is_open() const noexcept { return session_open_; }

    /// Get current offset.
    int64_t current_offset() const noexcept { return current_offset_; }

    /// Cancel current operation.
    void cancel() { cancelled_ = true; }
    bool is_cancelled() const noexcept { return cancelled_; }

    /// Get config.
    const FakeProviderConfig& config() const noexcept { return config_; }

private:
    FakeProviderConfig config_;
    kivo::p3::source_model::ProviderIdentity identity_;
    bool session_open_{false};
    int64_t current_offset_{0};
    bool cancelled_{false};
};

/// FakeEmbyProvider simulates Emby server behavior.
class FakeEmbyProvider : public FakeProvider
{
public:
    FakeEmbyProvider();
    /// Emby-specific: playback info.
    struct PlaybackInfo
    {
        std::string container{"mp4"};
        bool direct_play_supported{true};
        bool direct_stream_supported{true};
        bool transcoding_supported{false};
        std::string video_codec{"h264"};
        std::string audio_codec{"aac"};
    };
    PlaybackInfo get_playback_info() const { return playback_info_; }
private:
    PlaybackInfo playback_info_;
};

/// FakeJellyfinProvider simulates Jellyfin (distinct from Emby).
class FakeJellyfinProvider : public FakeProvider
{
public:
    FakeJellyfinProvider();
    struct PlaybackInfo
    {
        std::string container{"mkv"};
        bool direct_play_supported{true};
        std::string video_codec{"h265"};
        std::string audio_codec{"opus"};
    };
    PlaybackInfo get_playback_info() const { return playback_info_; }
    /// Jellyfin must have distinct evidence kind from Emby.
    std::string evidence_kind() const { return "jellyfin"; }
private:
    PlaybackInfo playback_info_;
};

/// FakeAlistProvider simulates Alist file/direct-link flow.
class FakeAlistProvider : public FakeProvider
{
public:
    FakeAlistProvider();
    struct DownloadLink
    {
        std::string redacted_url{};
        int64_t expiry_ns{0};
        std::string required_header{};
        bool is_signed{false};
    };
    DownloadLink resolve_download_link(const std::string& path);
    bool is_link_expired(const DownloadLink& link, int64_t now_ns) const noexcept
    { return now_ns >= link.expiry_ns; }
private:
    int64_t link_counter_{0};
};

/// FakeHlsProvider simulates HLS manifest without segment playback.
class FakeHlsProvider : public FakeProvider
{
public:
    FakeHlsProvider();
    struct ManifestInfo
    {
        int32_t variant_count{3};
        bool is_live{false};
        bool is_vod{true};
        bool has_encryption{false};
        bool has_subtitle_rendition{false};
        bool has_audio_rendition{false};
    };
    ManifestInfo get_manifest_info() const { return manifest_info_; }
    /// HLS must NOT have segment scheduler.
    bool has_segment_scheduler() const noexcept { return false; }
private:
    ManifestInfo manifest_info_;
};

/// FakeDashProvider simulates DASH MPD without ABR.
class FakeDashProvider : public FakeProvider
{
public:
    FakeDashProvider();
    struct MpdInfo
    {
        int32_t period_count{1};
        int32_t adaptation_set_count{2};
        int32_t representation_count{3};
        bool is_live{false};
        bool has_content_protection{false};
    };
    MpdInfo get_mpd_info() const { return mpd_info_; }
    /// DASH must NOT have ABR execution.
    bool has_abr_execution() const noexcept { return false; }
private:
    MpdInfo mpd_info_;
};

/// FakePlaylistProvider simulates simple playlist (M3U/M3U8/PLS).
class FakePlaylistProvider : public FakeProvider
{
public:
    FakePlaylistProvider();
    struct PlaylistInfo
    {
        std::string kind{"m3u"};
        int32_t entry_count{3};
        int32_t max_recursion_depth{5};
        bool allow_nested_playlist{true};
        bool is_hls{false};
    };
    PlaylistInfo get_playlist_info() const { return playlist_info_; }
    /// Check if #EXTM3U present but not HLS tags → simple playlist.
    static bool is_simple_m3u8(const std::string& content);
private:
    PlaylistInfo playlist_info_;
};

} // namespace kivo::p3::providers::fake
