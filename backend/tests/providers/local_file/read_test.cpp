#include "kivo/video/providers/local_file/local_file_provider.hpp"
#include "source_core/test_helpers.hpp"
#include <fstream>
#include <cstdio>
#include <cstdlib>

using namespace kivo::video::source_core;
using namespace kivo::video::source_core::test;
namespace lf = kivo::video::providers::local_file;

static std::string mk(const std::string& c) {
    char b[256]; std::snprintf(b,sizeof(b),"k_r_%d.tmp",std::rand());
    char abs[512]; _fullpath(abs,b,512); std::string p=abs;
    std::ofstream f(p,std::ios::binary); f.write(c.data(),static_cast<std::streamsize>(c.size())); return p;
}

int main() {
    auto tmp = mk("abcdef");
    lf::LocalFileSessionStore store;
    auto orr = lf::LocalFileProvider::open(SourceOpenRequest{SourceUri::from_raw(tmp)}, store);
    CHECK_TRUE(orr.is_success());
    auto sid = orr.session_if_success()->session_id;

    // 1. sequential read 3 => bytes="abc", current_offset=3
    SourceReadRequest r1{sid, std::nullopt, 3};
    auto rr1 = lf::LocalFileProvider::read(r1, store);
    CHECK_TRUE(rr1.is_success());
    CHECK_EQ(rr1.bytes_read, 3ULL);
    CHECK_EQ(rr1.bytes.size(), 3ULL);
    CHECK_EQ(rr1.bytes[0], (std::uint8_t)'a');
    CHECK_EQ(rr1.bytes[1], (std::uint8_t)'b');
    CHECK_EQ(rr1.bytes[2], (std::uint8_t)'c');

    // After sequential read, offset should be 3
    auto snap = store.snapshot(sid);
    CHECK_TRUE(snap.has_value());
    CHECK_EQ(snap->current_offset, 3ULL);

    // 2. positioned read offset 4 size 1 => bytes="e", offset stays 3
    SourceReadRequest r2{sid, 4ULL, 1};
    auto rr2 = lf::LocalFileProvider::read(r2, store);
    CHECK_TRUE(rr2.is_success());
    CHECK_EQ(rr2.bytes_read, 1ULL);
    CHECK_EQ(rr2.bytes[0], (std::uint8_t)'e');
    auto snap2 = store.snapshot(sid);
    CHECK_EQ(snap2->current_offset, 3ULL); // positioned does not update offset

    // 3. sequential read 3 => bytes="def", offset=6
    SourceReadRequest r3{sid, std::nullopt, 3};
    auto rr3 = lf::LocalFileProvider::read(r3, store);
    CHECK_TRUE(rr3.is_success());
    CHECK_EQ(rr3.bytes_read, 3ULL);
    CHECK_EQ(rr3.bytes[0], (std::uint8_t)'d');
    auto snap3 = store.snapshot(sid);
    CHECK_EQ(snap3->current_offset, 6ULL);

    // 4. over-read offset 100 => eof
    SourceReadRequest r4{sid, 100ULL, 1};
    auto rr4 = lf::LocalFileProvider::read(r4, store);
    CHECK_TRUE(rr4.eof);
    CHECK_EQ(rr4.bytes_read, 0ULL);

    // 5. zero read does not change offset
    auto before = store.snapshot(sid)->current_offset;
    SourceReadRequest zr{sid, std::nullopt, 0};
    auto zrr = lf::LocalFileProvider::read(zr, store);
    CHECK_TRUE(zrr.is_success());
    CHECK_EQ(zrr.bytes_read, 0ULL);
    auto after = store.snapshot(sid)->current_offset;
    CHECK_EQ(after, before);

    std::remove(tmp.c_str());
    return 0;
}
