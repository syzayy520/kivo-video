#include "test_fixture.hpp"
#include "source_core/test_helpers.hpp"
using namespace kivo::video::source_core; using namespace kivo::video::source_core::test;
int main(){
    auto fx=make_fixture(FixtureMode::auth_required_401,6);
    ph::HttpRangeSessionStore s;
    auto rr=ph::HttpRangeProvider::open(SourceOpenRequest{SourceUri::from_raw("http://x.com/f")},fx,s);
    CHECK_TRUE(rr.is_failure());
    return 0;
}
