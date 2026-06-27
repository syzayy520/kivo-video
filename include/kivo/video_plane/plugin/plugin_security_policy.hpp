// plugin_security_policy.hpp — GATE-035: P5A plugin security policy
#pragma once
#include <cstdint>
#include <string>

namespace kivo::video_plane::plugin {

enum class PluginTrustLevel {
    Trusted,                                // signed and verified plugin
    Sandbox,                                // sandboxed plugin
    Untrusted,                              // untrusted plugin
    Blocked                                 // blocked plugin
};

struct PluginSecurityPolicy {
    PluginTrustLevel default_trust{PluginTrustLevel::Blocked};
    bool require_signature{true};           // require plugin signature
    bool require_sandbox{true};             // require sandbox execution
    std::string allowed_api_surface;        // allowed API surface description
    bool operator==(PluginSecurityPolicy const&) const = default;
};

}  // namespace kivo::video_plane::plugin
