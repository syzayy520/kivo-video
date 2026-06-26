#include "test_fixture.hpp"
#include "source_core/test_helpers.hpp"
using namespace kivo::video::source_core; using namespace kivo::video::source_core::test;
int main(){
    auto fx=make_fixture(FixtureMode::redirect_ok,6);
    ph::HttpRangeSessionStore s;
    auto rr=ph::HttpRangeProvider::open(SourceOpenRequest{SourceUri::from_raw("http://x.com/f")},fx,s);
    // Redirect 301 -> provider handles redirect loop internally, may fail or retry
    (void)rr; return 0;
}
