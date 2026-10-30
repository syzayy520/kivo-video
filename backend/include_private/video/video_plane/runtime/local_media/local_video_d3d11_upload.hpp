#pragma once

#include <cstdint>
#include <memory>
#include <string>

#include "kivo/cinema_engine/decode_core/decoded_frame.hpp"

namespace kivo::video::video_plane::runtime::local_media {

struct LocalVideoD3d11UploadStats {
    bool available{false};
    bool initialized{false};
    bool uploaded{false};
    std::uint64_t frames_uploaded{0};
    std::uint64_t upload_failures{0};
    std::uint64_t last_upload_elapsed_ms{0};
    std::uint64_t max_upload_elapsed_ms{0};
    std::string device_type;
    std::string error;
};

class LocalVideoD3d11Upload {
public:
    LocalVideoD3d11Upload();
    ~LocalVideoD3d11Upload();

    LocalVideoD3d11Upload(const LocalVideoD3d11Upload&) = delete;
    LocalVideoD3d11Upload& operator=(const LocalVideoD3d11Upload&) = delete;

    [[nodiscard]] bool upload(const kivo::cinema_engine::DecodedFrame& frame) noexcept;
    void reset_texture() noexcept;
    void close() noexcept;

    [[nodiscard]] const LocalVideoD3d11UploadStats& stats() const noexcept { return stats_; }

private:
    struct Impl;

    [[nodiscard]] bool ensure_initialized() noexcept;
    [[nodiscard]] bool ensure_texture(const kivo::cinema_engine::DecodedFrame& frame) noexcept;

    std::unique_ptr<Impl> impl_;
    LocalVideoD3d11UploadStats stats_{};
    std::int32_t texture_width_{0};
    std::int32_t texture_height_{0};
    std::string texture_pixel_format_;
};

}  // namespace kivo::video::video_plane::runtime::local_media
