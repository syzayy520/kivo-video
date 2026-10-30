#pragma once

#include <cstdint>
#include <string>

namespace kivo::p3::source_model {

/// SourceOriginKind describes item origin.
enum class SourceOriginKind
{
    local_file,
    local_network_share,
    remote_http,
    remote_webdav,
    remote_provider_emby,
    remote_provider_jellyfin,
    remote_provider_alist,
    remote_cloud_drive,
    remote_hls,
    remote_dash,
    remote_playlist,
    unsupported_protocol
};

/// ProviderFamily describes provider family (not transport/delivery/container).
enum class ProviderFamily
{
    local_file,
    http,
    webdav,
    emby,
    jellyfin,
    alist,
    cloud_drive,
    hls,
    dash,
    playlist,
    smb,
    upnp_dlna,
    ftp,
    rtsp,
    rtmp,
    unsupported
};

/// SourceTransportKind describes transport.
enum class SourceTransportKind
{
    local_fs,
    http_range,
    webdav_range,
    provider_resolved_url,
    provider_direct_stream,
    manifest_segment,
    unsupported
};

/// MediaDeliveryKind describes delivery.
enum class MediaDeliveryKind
{
    progressive_file,
    progressive_http,
    hls_manifest,
    dash_manifest,
    playlist,
    unsupported
};

/// ContainerFormat comes from probe/fast probe/provider metadata cross-validation.
enum class ContainerFormat
{
    mp4,
    mkv,
    avi,
    mov,
    flv,
    mpegts,
    ebml,
    webm,
    unknown
};

} // namespace kivo::p3::source_model
