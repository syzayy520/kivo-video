#include "test_fixture.hpp"
#include "source_core/test_helpers.hpp"
using namespace kivo::video::source_core; using namespace kivo::video::source_core::test;

int main() {
    auto fx=make_fixture(FixtureMode::range_ok,6,"abcdef");
    ph::HttpRangeSessionStore store;
    auto orr=ph::HttpRangeProvider::open(SourceOpenRequest{SourceUri::from_raw("http://x.com/f")},fx,store);
    CHECK_TRUE(orr.is_success());
    auto sid=orr.session_if_success()->session_id;

    // Sequential read 3 = "abc"
    SourceReadRequest r1{sid,std::nullopt,3};
    auto rr1=ph::HttpRangeProvider::read(r1,fx,store);
    CHECK_TRUE(rr1.is_success()); CHECK_EQ(rr1.bytes_read,3ULL); CHECK_EQ(rr1.bytes[0],(std::uint8_t)'a');

    // Positioned read offset 4 size 1 = "e", offset stays
    SourceReadRequest r2{sid,4ULL,1};
    auto rr2=ph::HttpRangeProvider::read(r2,fx,store);
    CHECK_TRUE(rr2.is_success()); CHECK_EQ(rr2.bytes[0],(std::uint8_t)'e');

    // EOF over-read
    SourceReadRequest r3{sid,100ULL,1};
    auto rr3=ph::HttpRangeProvider::read(r3,fx,store);
    CHECK_TRUE(rr3.eof); CHECK_EQ(rr3.bytes_read,0ULL);

    // Zero read
    auto before=store.snapshot(sid)->current_offset;
    SourceReadRequest zr{sid,std::nullopt,0};
    auto zrr=ph::HttpRangeProvider::read(zr,fx,store);
    CHECK_TRUE(zrr.is_success()); CHECK_EQ(zrr.bytes_read,0ULL);
    CHECK_EQ(store.snapshot(sid)->current_offset,before);
    return 0;
}
