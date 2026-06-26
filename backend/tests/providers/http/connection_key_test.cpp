#include "test_fixture.hpp"
#include "source_core/test_helpers.hpp"
using namespace kivo::video::source_core; using namespace kivo::video::source_core::test;
int main(){auto fx=make_fixture(FixtureMode::range_ok,6);ph::HttpRangeSessionStore s;auto o1=ph::HttpRangeProvider::open(SourceOpenRequest{SourceUri::from_raw("http://a.com:80/f")},fx,s);CHECK_TRUE(o1.is_success());auto o2=ph::HttpRangeProvider::open(SourceOpenRequest{SourceUri::from_raw("http://a.com/f")},fx,s);CHECK_TRUE(o2.is_success());CHECK_NEQ(o1.session_if_success()->session_id.value,o2.session_if_success()->session_id.value);return 0;}
