#include "test_fixture.hpp"
#include "source_core/test_helpers.hpp"
using namespace kivo::video::source_core; using namespace kivo::video::source_core::test;
int main(){auto fxv=make_fixture(FixtureMode::one_byte_valid,1,"x");ph::HttpRangeSessionStore s;auto rv=ph::HttpRangeProvider::open(SourceOpenRequest{SourceUri::from_raw("http://x.com/a")},fxv,s);CHECK_TRUE(rv.is_success());ph::HttpRangeSessionStore s2;auto fxn=make_fixture(FixtureMode::one_byte_nonconformant,1);auto rn=ph::HttpRangeProvider::open(SourceOpenRequest{SourceUri::from_raw("http://x.com/b")},fxn,s2);/* nonconformant 416 */ return 0;}
