#include "test_fixture.hpp"
#include "source_core/test_helpers.hpp"
using namespace kivo::video::source_core; using namespace kivo::video::source_core::test;
int main(){auto fx=make_fixture(FixtureMode::range_ok,6);ph::HttpRangeSessionStore s;auto o=ph::HttpRangeProvider::open(SourceOpenRequest{SourceUri::from_raw("http://x.com/f")},fx,s);CHECK_TRUE(o.is_success());auto id=o.session_if_success()->session_id;ph::HttpRangeProvider::read(SourceReadRequest{id,std::nullopt,3},fx,s);ph::HttpRangeProvider::close(id,s);auto ev=ph::HttpRangeProvider::get_evidence(id,s);CHECK_TRUE(!ev.items.empty()); return 0;}
