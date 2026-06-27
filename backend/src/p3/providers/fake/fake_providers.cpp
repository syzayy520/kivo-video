#include "kivo/p3/providers/fake/fake_providers.hpp"

#include <algorithm>
#include <cstring>

namespace kivo::p3::providers::fake {

FakeProvider::FakeProvider(FakeProviderConfig config)
    : config_(std::move(config))
{
    identity_.schema_version = "p3.source_model.provider_identity.v1";
    identity_.provider_id = config_.provider_id;
    switch (config_.kind)
    {
        case FakeProviderKind::http:    identity_.family = kivo::p3::source_model::ProviderFamily::http; break;
        case FakeProviderKind::webdav:  identity_.family = kivo::p3::source_model::ProviderFamily::webdav; break;
        case FakeProviderKind::emby:    identity_.family = kivo::p3::source_model::ProviderFamily::emby; break;
        case FakeProviderKind::jellyfin: identity_.family = kivo::p3::source_model::ProviderFamily::jellyfin; break;
        case FakeProviderKind::alist:   identity_.family = kivo::p3::source_model::ProviderFamily::alist; break;
        case FakeProviderKind::hls:     identity_.family = kivo::p3::source_model::ProviderFamily::hls; break;
        case FakeProviderKind::dash:    identity_.family = kivo::p3::source_model::ProviderFamily::dash; break;
        case FakeProviderKind::playlist: identity_.family = kivo::p3::source_model::ProviderFamily::playlist; break;
    }
    identity_.redacted_host = config_.redacted_host;
}

bool FakeProvider::open_session(const kivo::p3::sessions::SourceSessionPlan& plan)
{
    if (cancelled_) return false;
    if (!plan.is_valid()) return false;
    if (config_.simulate_failure) return false;
    session_open_ = true;
    current_offset_ = 0;
    return true;
}

FakeProviderResponse FakeProvider::read(int64_t offset, int64_t length)
{
    FakeProviderResponse resp;
    if (cancelled_)
    {
        resp.is_cancelled = true;
        resp.error_reason = "cancelled";
        return resp;
    }
    if (!session_open_)
    {
        resp.error_reason = "session_not_open";
        return resp;
    }
    if (config_.simulate_timeout)
    {
        resp.error_reason = "timeout";
        return resp;
    }
    if (config_.simulate_cancel)
    {
        cancelled_ = true;
        resp.is_cancelled = true;
        resp.error_reason = "cancelled_during_read";
        return resp;
    }
    if (offset < 0 || length <= 0)
    {
        resp.error_reason = "invalid_request";
        return resp;
    }
    if (offset >= config_.simulated_content_length)
    {
        resp.error_reason = "out_of_range";
        return resp;
    }
    int64_t available = config_.simulated_content_length - offset;
    int64_t to_read = std::min(length, available);
    resp.data.resize(static_cast<size_t>(to_read));
    for (size_t i = 0; i < resp.data.size(); ++i)
    {
        resp.data[i] = static_cast<uint8_t>((offset + static_cast<int64_t>(i)) & 0xFF);
    }
    resp.success = true;
    resp.content_length = config_.simulated_content_length;
    resp.status_code = 206;
    current_offset_ = offset + to_read;
    return resp;
}

bool FakeProvider::seek(int64_t offset)
{
    if (!session_open_ || cancelled_) return false;
    if (offset < 0 || offset >= config_.simulated_content_length) return false;
    current_offset_ = offset;
    return true;
}

bool FakeProvider::close_session()
{
    if (!session_open_) return true; // idempotent
    session_open_ = false;
    current_offset_ = 0;
    return true;
}

// ─── FakeEmbyProvider ───

FakeEmbyProvider::FakeEmbyProvider()
    : FakeProvider([]{
        FakeProviderConfig c;
        c.kind = FakeProviderKind::emby;
        c.provider_id = "fake-emby-001";
        c.redacted_host = "redacted-emby-host";
        c.port = 8096;
        c.auth_required = true;
        c.redacted_credential_scope = "emby-scope";
        return c;
      }())
{
    playback_info_.container = "mp4";
    playback_info_.direct_play_supported = true;
    playback_info_.direct_stream_supported = true;
    playback_info_.transcoding_supported = false;
    playback_info_.video_codec = "h264";
    playback_info_.audio_codec = "aac";
}

// ─── FakeJellyfinProvider ───

FakeJellyfinProvider::FakeJellyfinProvider()
    : FakeProvider([]{
        FakeProviderConfig c;
        c.kind = FakeProviderKind::jellyfin;
        c.provider_id = "fake-jellyfin-001";
        c.redacted_host = "redacted-jellyfin-host";
        c.port = 8097;
        c.auth_required = true;
        c.redacted_credential_scope = "jellyfin-scope";
        return c;
      }())
{
    playback_info_.container = "mkv";
    playback_info_.direct_play_supported = true;
    playback_info_.video_codec = "h265";
    playback_info_.audio_codec = "opus";
}

// ─── FakeAlistProvider ───

FakeAlistProvider::FakeAlistProvider()
    : FakeProvider([]{
        FakeProviderConfig c;
        c.kind = FakeProviderKind::alist;
        c.provider_id = "fake-alist-001";
        c.redacted_host = "redacted-alist-host";
        c.port = 5244;
        c.auth_required = true;
        c.redacted_credential_scope = "alist-scope";
        return c;
      }())
{
}

FakeAlistProvider::DownloadLink FakeAlistProvider::resolve_download_link(const std::string& path)
{
    DownloadLink link;
    link.redacted_url = "redacted-alist-link-" + std::to_string(++link_counter_);
    link.expiry_ns = 300'000'000'000LL; // 300 seconds
    link.required_header = "Authorization: <redacted>";
    link.is_signed = true;
    return link;
}

// ─── FakeHlsProvider ───

FakeHlsProvider::FakeHlsProvider()
    : FakeProvider([]{
        FakeProviderConfig c;
        c.kind = FakeProviderKind::hls;
        c.provider_id = "fake-hls-001";
        c.redacted_host = "redacted-hls-host";
        c.port = 443;
        c.supports_range_read = false;
        c.supports_seek = false;
        return c;
      }())
{
    manifest_info_.variant_count = 3;
    manifest_info_.is_live = false;
    manifest_info_.is_vod = true;
    manifest_info_.has_encryption = false;
}

// ─── FakeDashProvider ───

FakeDashProvider::FakeDashProvider()
    : FakeProvider([]{
        FakeProviderConfig c;
        c.kind = FakeProviderKind::dash;
        c.provider_id = "fake-dash-001";
        c.redacted_host = "redacted-dash-host";
        c.port = 443;
        c.supports_range_read = false;
        c.supports_seek = false;
        return c;
      }())
{
    mpd_info_.period_count = 1;
    mpd_info_.adaptation_set_count = 2;
    mpd_info_.representation_count = 3;
    mpd_info_.is_live = false;
    mpd_info_.has_content_protection = false;
}

// ─── FakePlaylistProvider ───

FakePlaylistProvider::FakePlaylistProvider()
    : FakeProvider([]{
        FakeProviderConfig c;
        c.kind = FakeProviderKind::playlist;
        c.provider_id = "fake-playlist-001";
        c.redacted_host = "";
        c.port = 0;
        c.supports_range_read = false;
        c.supports_seek = false;
        return c;
      }())
{
    playlist_info_.kind = "m3u";
    playlist_info_.entry_count = 3;
    playlist_info_.max_recursion_depth = 5;
    playlist_info_.allow_nested_playlist = true;
    playlist_info_.is_hls = false;
}

bool FakePlaylistProvider::is_simple_m3u8(const std::string& content)
{
    // #EXTM3U is necessary but not sufficient for HLS.
    bool has_extm3u = content.find("#EXTM3U") != std::string::npos;
    // HLS-specific tags that indicate HLS, not simple playlist.
    bool has_hls_tags = content.find("#EXT-X-") != std::string::npos;
    // Simple playlist: has EXTM3U but no HLS-specific tags.
    return has_extm3u && !has_hls_tags;
}

} // namespace kivo::p3::providers::fake
