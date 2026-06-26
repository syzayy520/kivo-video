#include "test_fixture.hpp"
#include "source_core/test_helpers.hpp"
using namespace kivo::video::source_core; using namespace kivo::video::source_core::test;
int main(){
    auto fx=make_fixture(FixtureMode::compressed_range,6);
    ph::HttpRangeSessionStore s;
    auto rr=ph::HttpRangeProvider::open(SourceOpenRequest{SourceUri::from_raw("http://x.com/f")},fx,s);
    CHECK_TRUE(rr.is_failure()); // compressed rejected
    auto fx2=make_fixture(FixtureMode::multipart_range,6);
    ph::HttpRangeSessionStore s2;
    auto r2=ph::HttpRangeProvider::open(SourceOpenRequest{SourceUri::from_raw("http://x.com/g")},fx2,s2);
    CHECK_TRUE(r2.is_failure()); // multipart rejected
    return 0;
}
