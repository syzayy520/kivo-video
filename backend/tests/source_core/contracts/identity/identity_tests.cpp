#include "kivo/video/source_core/contracts/identity/source_identity.hpp"
#include "source_core/test_helpers.hpp"

using namespace kivo::video::source_core;
using namespace kivo::video::source_core::test;

int main() {
    SourceIdentity id;
    CHECK_EQ(id.id.value, 0ULL);
    CHECK_EQ(id.kind, ProviderKind::unknown);

    id.id = SourceIdentityId{42};
    id.kind = ProviderKind::local_file;
    id.safe_label = "test_label";
    CHECK_EQ(id.id.value, 42ULL);
    CHECK_EQ(id.kind, ProviderKind::local_file);

    SourceIdentityId a{1};
    SourceIdentityId b{1};
    SourceIdentityId c{2};
    CHECK_EQ(a, b);
    CHECK_NEQ(a, c);
    return 0;
}
