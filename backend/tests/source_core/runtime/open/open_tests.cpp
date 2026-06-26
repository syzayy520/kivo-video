#include "kivo/video/source_core/runtime/open/source_open_request.hpp"
#include "kivo/video/source_core/runtime/open/source_open_result.hpp"
#include "source_core/test_helpers.hpp"

using namespace kivo::video::source_core;
using namespace kivo::video::source_core::test;

int main() {
    auto uri = SourceUri::from_raw("https://example.com/video.mp4");
    SourceOpenRequest req;
    req.uri = std::move(uri);

    SourceSession s;
    s.session_id = SourceSessionId{1};
    s.session_state = SourceSessionState::open;
    auto result = SourceOpenResult::success(s);
    CHECK_TRUE(result.is_success());
    CHECK_TRUE(!result.is_failure());
    CHECK_TRUE(result.session_if_success() != nullptr);
    CHECK_EQ(result.session_if_success()->session_id.value, 1ULL);
    CHECK_EQ(result.session_if_success()->session_state, SourceSessionState::open);
    CHECK_TRUE(result.error_if_failure() == nullptr);

    SourceError err{SourceErrorCode::auth_failed, "bad credentials"};
    auto fr = SourceOpenResult::failure(err);
    CHECK_TRUE(fr.is_failure());
    CHECK_TRUE(!fr.is_success());
    CHECK_TRUE(fr.error_if_failure() != nullptr);
    CHECK_EQ(fr.error_if_failure()->code, SourceErrorCode::auth_failed);
    CHECK_TRUE(fr.session_if_success() == nullptr);
    return 0;
}
