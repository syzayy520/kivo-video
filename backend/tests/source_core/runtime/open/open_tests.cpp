#include "kivo/video/source_core/runtime/open/source_open_request.hpp"
#include "kivo/video/source_core/runtime/open/source_open_result.hpp"

#include <cassert>

using namespace kivo::video::source_core;

int main() {
    // Open request with raw URI
    auto uri = SourceUri::from_raw("https://example.com/video.mp4");
    SourceOpenRequest req;
    req.uri = std::move(uri);
    
    // Open result strong wrapper: success holds SourceSession
    SourceSession session;
    session.id = SourceSessionId{1};
    session.state = SourceSessionState::open;
    
    auto result = SourceOpenResult::success(session);
    assert(result.is_success());
    assert(!result.is_failure());
    assert(result.session() != nullptr);
    assert(result.session()->id.value == 1);
    assert(result.session()->state == SourceSessionState::open);
    assert(result.error() == nullptr);
    
    // Open result: failure holds SourceError
    SourceError err{SourceErrorCode::auth_failed, "bad credentials"};
    auto fail_result = SourceOpenResult::failure(err);
    assert(fail_result.is_failure());
    assert(!fail_result.is_success());
    assert(fail_result.error() != nullptr);
    assert(fail_result.error()->code == SourceErrorCode::auth_failed);
    assert(fail_result.session() == nullptr);
    
    return 0;
}
