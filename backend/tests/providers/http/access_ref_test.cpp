#include "test_fixture.hpp"
#include "source_core/test_helpers.hpp"
using namespace kivo::video::source_core; using namespace kivo::video::source_core::test;
int main(){auto fx=make_fixture(FixtureMode::range_ok,6);ph::HttpRangeSessionStore s;auto o=ph::HttpRangeProvider::open(SourceOpenRequest{SourceUri::from_raw("http://x.com/f")},fx,s);CHECK_TRUE(o.is_success());auto id=o.session_if_success()->session_id;auto dpi=ph::HttpRangeProvider::make_direct_play_input(id,s);CHECK_TRUE(dpi.is_success());auto ref=dpi.input->access_ref;CHECK_EQ(ref.session_id.value,id.value);ph::HttpRangeProvider::close(id,s);auto dpi2=ph::HttpRangeProvider::make_direct_play_input(id,s);CHECK_TRUE(!dpi2.is_success());return 0;}
