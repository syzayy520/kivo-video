#include "test_fixture.hpp"
#include "source_core/test_helpers.hpp"
using namespace kivo::video::source_core; using namespace kivo::video::source_core::test;

int main() {
    auto fx=make_fixture(FixtureMode::range_ok,6,"abcdef");
    ph::HttpRangeSessionStore store;
    auto rr=ph::HttpRangeProvider::open(SourceOpenRequest{SourceUri::from_raw("http://example.com/f.mp4")},fx,store);
    CHECK_TRUE(rr.is_success());
    CHECK_EQ(rr.session_if_success()->session_state,SourceSessionState::open);
    CHECK_EQ(rr.session_if_success()->provider_kind,ProviderKind::http_file);

    // Invalid URL
    auto r2=ph::HttpRangeProvider::open(SourceOpenRequest{SourceUri::from_raw("not_a_url")},fx,store);
    CHECK_TRUE(r2.is_failure());
    return 0;
}
