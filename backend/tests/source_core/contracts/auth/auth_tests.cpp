#include "kivo/video/source_core/contracts/auth/source_auth.hpp"

#include <cassert>
#include <cstdint>

using namespace kivo::video::source_core;

int main() {
    // Default auth info
    SourceAuthInfo auth;
    assert(auth.requirement == AuthRequirement::unknown);
    assert(auth.kind == AuthKind::unknown);
    
    // SourceSecretRef is opaque, no credential values
    SourceSecretRef secret{42};
    assert(secret.opaque_id == 42);
    
    // Set auth requirements
    auth.requirement = AuthRequirement::required;
    auth.kind = AuthKind::bearer_token;
    auth.secret = SourceSecretRef{100};
    
    assert(auth.requirement == AuthRequirement::required);
    assert(auth.kind == AuthKind::bearer_token);
    assert(auth.secret.opaque_id == 100);
    
    // Enum values are distinct
    assert(AuthRequirement::not_required != AuthRequirement::required);
    assert(AuthKind::basic != AuthKind::bearer_token);
    
    return 0;
}
