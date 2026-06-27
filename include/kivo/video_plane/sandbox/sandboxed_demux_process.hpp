#pragma once

#include <string>

namespace kivo::video_plane::sandbox {

struct SandboxedDemuxProcess {
    std::string process_id;
    std::string sandbox_token;
    std::string broker_contract_id;
    bool operator==(SandboxedDemuxProcess const&) const = default;
};

}  // namespace kivo::video_plane::sandbox
