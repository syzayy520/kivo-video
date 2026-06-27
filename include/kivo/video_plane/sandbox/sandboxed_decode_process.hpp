#pragma once

#include <string>

namespace kivo::video_plane::sandbox {

struct SandboxedDecodeProcess {
    std::string process_id;
    std::string sandbox_token;
    bool operator==(SandboxedDecodeProcess const&) const = default;
};

}  // namespace kivo::video_plane::sandbox
