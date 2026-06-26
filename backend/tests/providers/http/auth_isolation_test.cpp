#include "test_fixture.hpp"
#include "source_core/test_helpers.hpp"
using namespace kivo::video::source_core; using namespace kivo::video::source_core::test;
int main(){auto fx=make_fixture(FixtureMode::range_ok,6);ph::HttpRangeSessionStore s;auto o1=ph::HttpRangeProvider::open(SourceOpenRequest{SourceUri::from_raw("http://x.com/f")},fx,s);CHECK_TRUE(o1.is_success());auto id1=o1.session_if_success()->session_id;auto o2=ph::HttpRangeProvider::open(SourceOpenRequest{SourceUri::from_raw("http://x.com/f2")},fx,s);CHECK_TRUE(o2.is_success());auto id2=o2.session_if_success()->session_id;CHECK_NEQ(id1.value,id2.value);return 0;}
