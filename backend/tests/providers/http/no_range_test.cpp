#include "test_fixture.hpp"
#include "source_core/test_helpers.hpp"
using namespace kivo::video::source_core; using namespace kivo::video::source_core::test;
int main() {
    auto fx=make_fixture(FixtureMode::no_range_200,6);
    ph::HttpRangeSessionStore store;
    auto rr=ph::HttpRangeProvider::open(SourceOpenRequest{SourceUri::from_raw("http://x.com/f")},fx,store);
    CHECK_TRUE(rr.is_failure()); // 200 no-range rejected
    return 0;
}
