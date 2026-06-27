#pragma once

#include <string>

namespace kivo::video_plane::capability {

struct CapabilityVerificationScope {
    std::string platform;           // platform name
    std::string os_version;         // OS version
    std::string driver_version;     // driver version
    std::string gpu_vendor;         // GPU vendor
    std::string gpu_device_id;      // GPU device ID
    std::string adapter_luid;       // adapter LUID (opaque)
    std::string content_profile;    // content profile
    std::string container_profile;  // container profile
    std::string codec_profile;      // codec profile
    std::string pixel_format;       // pixel format
    std::string decode_path;        // decode path
    std::string render_path;        // render path
    std::string output_path;        // output path
    std::string display_mode;       // display mode
    std::string hdr_mode;           // HDR mode
    std::string vrr_mode;           // VRR mode
    std::string test_lab_id;        // test lab ID
    std::string verification_level; // verification level
    std::string evidence_id;        // linked evidence ID
    bool operator==(CapabilityVerificationScope const&) const = default;
};

}  // namespace kivo::video_plane::capability
