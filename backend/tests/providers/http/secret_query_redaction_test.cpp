#include "test_fixture.hpp"
#include "source_core/test_helpers.hpp"
#include <string>
using namespace kivo::video::source_core; using namespace kivo::video::source_core::test;
int main(){auto fx=make_fixture(FixtureMode::range_ok,6);ph::HttpRangeSessionStore s;auto o=ph::HttpRangeProvider::open(SourceOpenRequest{SourceUri::from_raw("http://x.com/f?token=secret&sig=abc123")},fx,s);CHECK_TRUE(o.is_success());auto ev=ph::HttpRangeProvider::get_evidence(o.session_if_success()->session_id,s);for(auto&i:ev.items){CHECK_TRUE(i.safe_description.find("token=secret")==std::string::npos);CHECK_TRUE(i.safe_description.find("sig=abc123")==std::string::npos);}return 0;}
