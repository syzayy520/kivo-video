#include "test_fixture.hpp"
#include "source_core/test_helpers.hpp"
#include <thread>
using namespace kivo::video::source_core; using namespace kivo::video::source_core::test;
int main(){auto fx=make_fixture(FixtureMode::range_ok,6,"abcdef");ph::HttpRangeSessionStore s;auto o=ph::HttpRangeProvider::open(SourceOpenRequest{SourceUri::from_raw("http://x.com/f")},fx,s);CHECK_TRUE(o.is_success());auto id=o.session_if_success()->session_id;std::thread t1([&]{auto r=ph::HttpRangeProvider::read(SourceReadRequest{id,0ULL,1},fx,s);CHECK_TRUE(r.is_success()||!r.is_success());});std::thread t2([&]{auto r=ph::HttpRangeProvider::read(SourceReadRequest{id,2ULL,1},fx,s);CHECK_TRUE(r.is_success()||!r.is_success());});t1.join();t2.join();return 0;}
