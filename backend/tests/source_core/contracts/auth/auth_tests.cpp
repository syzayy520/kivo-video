#include "kivo/video/source_core/contracts/auth/source_auth.hpp"
#include "source_core/test_helpers.hpp"

using namespace kivo::video::source_core;
using namespace kivo::video::source_core::test;

int main() {
    SourceAuthInfo auth;
    CHECK_EQ(auth.requirement, AuthRequirement::unknown);
    CHECK_EQ(auth.kind, AuthKind::unknown);

    SourceSecretRef secret{42};
    CHECK_EQ(secret.opaque_id, 42ULL);

    auth.requirement = AuthRequirement::required;
    auth.kind = AuthKind::bearer_token;
    auth.secret = SourceSecretRef{100};
    CHECK_EQ(auth.requirement, AuthRequirement::required);
    CHECK_EQ(auth.kind, AuthKind::bearer_token);
    CHECK_EQ(auth.secret.opaque_id, 100ULL);
    CHECK_NEQ(AuthRequirement::not_required, AuthRequirement::required);
    CHECK_NEQ(AuthKind::basic, AuthKind::bearer_token);
    return 0;
}
