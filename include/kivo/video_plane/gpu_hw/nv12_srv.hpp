// nv12_srv.hpp -- P5G G4: NV12 plane SRV (fake)
#pragma once
#include <cstdint>
#include <string>

namespace kivo::video_plane::gpu_hw {

struct Nv12SrvResult {
    bool luma_srv_created{false};
    bool chroma_srv_created{false};
    uint32_t luma_plane_width{0};
    uint32_t luma_plane_height{0};
    uint32_t chroma_plane_width{0};
    uint32_t chroma_plane_height{0};
    std::string failure_reason;
};

class FakeNv12SrvCreator {
public:
    Nv12SrvResult create_srvs(uint32_t width, uint32_t height) {
        Nv12SrvResult r;
        if (width == 0 || height == 0) {
            r.failure_reason = "invalid_dimensions";
            return r;
        }
        r.luma_srv_created = true;
        r.chroma_srv_created = true;
        r.luma_plane_width = width;
        r.luma_plane_height = height;
        // NV12 chroma is half width and half height (4:2:0)
        r.chroma_plane_width = width / 2;
        r.chroma_plane_height = height / 2;
        return r;
    }

    Nv12SrvResult simulate_failure() {
        Nv12SrvResult r;
        r.failure_reason = "srv_creation_failed";
        return r;
    }
};

}  // namespace kivo::video_plane::gpu_hw
