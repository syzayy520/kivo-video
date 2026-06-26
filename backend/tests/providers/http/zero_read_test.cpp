#include "test_fixture.hpp"
#include "source_core/test_helpers.hpp"
using namespace kivo::video::source_core; using namespace kivo::video::source_core::test;
int main(){auto fx=make_fixture(FixtureMode::range_ok,6);ph::HttpRangeSessionStore s;auto o=ph::HttpRangeProvider::open(SourceOpenRequest{SourceUri::from_raw("http://x.com/f")},fx,s);CHECK_TRUE(o.is_success());auto id=o.session_if_success()->session_id;auto ofs=s.snapshot(id)->current_offset;auto rr=ph::HttpRangeProvider::read(SourceReadRequest{id,std::nullopt,0},fx,s);CHECK_TRUE(rr.is_success());CHECK_EQ(rr.bytes_read,0ULL);CHECK_EQ(s.snapshot(id)->current_offset,ofs);return 0;}
