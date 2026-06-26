#include "test_fixture.hpp"
#include "source_core/test_helpers.hpp"
using namespace kivo::video::source_core; using namespace kivo::video::source_core::test;
int main() {
    auto fx=make_fixture(FixtureMode::empty_416,0);
    ph::HttpRangeSessionStore store;
    auto rr=ph::HttpRangeProvider::open(SourceOpenRequest{SourceUri::from_raw("http://x.com/f")},fx,store);
    CHECK_TRUE(rr.is_success());
    CHECK_EQ(rr.session_if_success()->session_state,SourceSessionState::open);
    // Seek to 0 only allowed
    auto sr=ph::HttpRangeProvider::seek(SourceSeekRequest{rr.session_if_success()->session_id,0,SeekOrigin::begin},store);
    CHECK_TRUE(sr.is_success());
    // Seek > 0 fails
    auto sf=ph::HttpRangeProvider::seek(SourceSeekRequest{rr.session_if_success()->session_id,1,SeekOrigin::begin},store);
    CHECK_TRUE(!sf.is_success());
    return 0;
}
