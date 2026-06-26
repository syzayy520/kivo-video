#include "test_fixture.hpp"
#include "source_core/test_helpers.hpp"
using namespace kivo::video::source_core; using namespace kivo::video::source_core::test;
int main(){auto fx=make_fixture(FixtureMode::range_ok,6,"abcdef");ph::HttpRangeSessionStore s;auto o=ph::HttpRangeProvider::open(SourceOpenRequest{SourceUri::from_raw("http://x.com/f")},fx,s);CHECK_TRUE(o.is_success());auto id=o.session_if_success()->session_id;auto rr1=ph::HttpRangeProvider::read(SourceReadRequest{id,std::nullopt,3},fx,s);CHECK_TRUE(rr1.is_success());auto rr2=ph::HttpRangeProvider::read(SourceReadRequest{id,std::nullopt,3},fx,s);CHECK_TRUE(rr2.is_success());CHECK_TRUE(rr1.bytes_read>0);return 0;}
