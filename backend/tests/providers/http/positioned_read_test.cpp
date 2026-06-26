#include "test_fixture.hpp"
#include "source_core/test_helpers.hpp"
using namespace kivo::video::source_core; using namespace kivo::video::source_core::test;
int main(){auto fx=make_fixture(FixtureMode::range_ok,6,"abcdef");ph::HttpRangeSessionStore s;auto o=ph::HttpRangeProvider::open(SourceOpenRequest{SourceUri::from_raw("http://x.com/f")},fx,s);CHECK_TRUE(o.is_success());auto id=o.session_if_success()->session_id;auto off1=s.snapshot(id)->current_offset;auto rr=ph::HttpRangeProvider::read(SourceReadRequest{id,4ULL,1},fx,s);CHECK_TRUE(rr.is_success());CHECK_EQ(rr.bytes[0],(std::uint8_t)'e');auto off2=s.snapshot(id)->current_offset;CHECK_EQ(off1,off2);return 0;}
