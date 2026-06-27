#pragma once

namespace kivo::video_plane::sandbox {

struct SandboxFileAccessPolicy {
    bool allow_local_file_system{true};
    bool allow_network{true};
    bool allow_temp_directory{true};
    bool deny_write_to_source_directory{true};
    bool deny_access_to_provider_credential{true};
    bool deny_access_to_drm_key_store{true};
    bool operator==(SandboxFileAccessPolicy const&) const = default;
};

}  // namespace kivo::video_plane::sandbox
