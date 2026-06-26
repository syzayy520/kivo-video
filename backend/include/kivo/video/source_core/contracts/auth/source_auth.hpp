#pragma once

#include <cstdint>
#include <string>

namespace kivo::video::source_core {

enum class AuthRequirement : std::uint8_t {
    not_required = 0,
    required = 1,
    unknown = 2
};

enum class AuthKind : std::uint8_t {
    none = 0,
    basic = 1,
    bearer_token = 2,
    api_key = 3,
    signed_url = 4,
    oauth2 = 5,
    unknown = 6
};

/// Opaque secret reference. Never stores real credential values.
struct SourceSecretRef {
    std::uint64_t opaque_id{0};

    // No raw token, password, API key, or cookie.
};

struct SourceAuthInfo {
    AuthRequirement requirement{AuthRequirement::unknown};
    AuthKind kind{AuthKind::unknown};
    SourceSecretRef secret;
};

}  // namespace kivo::video::source_core
