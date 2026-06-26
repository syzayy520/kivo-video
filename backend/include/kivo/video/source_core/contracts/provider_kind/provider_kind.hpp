#pragma once

#include <cstdint>

namespace kivo::video::source_core {

enum class ProviderKind : std::uint8_t {
    local_file = 1,
    smb = 2,
    nfs = 3,
    ftp = 4,
    ftps = 5,
    sftp = 6,
    http_file = 7,
    https_file = 8,
    http_stream = 9,
    https_stream = 10,
    webdav = 11,
    alist = 12,
    emby = 13,
    plex = 14,
    jellyfin = 15,
    google_drive = 16,
    onedrive = 17,
    s3_object = 18,
    minio_object = 19,
    iptv_m3u = 20,
    hls_live = 21,
    dlna_upnp = 22,

    unknown = 0
};

}  // namespace kivo::video::source_core
