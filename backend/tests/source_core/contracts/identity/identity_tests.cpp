#include "kivo/video/source_core/contracts/identity/source_identity.hpp"

#include <cassert>
#include <string>

using namespace kivo::video::source_core;

int main() {
    // Default constructed identity
    SourceIdentity id;
    assert(id.id.value == 0);
    assert(id.kind == ProviderKind::unknown);
    
    // Set fields
    id.id = SourceIdentityId{42};
    id.kind = ProviderKind::local_file;
    id.safe_label = "test_label";
    assert(id.id.value == 42);
    assert(id.kind == ProviderKind::local_file);
    
    // Safe debug output: no raw URI, no credentials
    // safe_label is fine for logs
    
    // SourceIdentityId comparison
    SourceIdentityId a{1};
    SourceIdentityId b{1};
    SourceIdentityId c{2};
    assert(a == b);
    assert(!(a == c));
    
    return 0;
}
