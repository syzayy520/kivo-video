#include "kivo/video/providers/local_file/local_file_provider.hpp"
#include "source_core/test_helpers.hpp"
#include <fstream>
#include <cstdio>
#include <cstdlib>

using namespace kivo::video::source_core;
using namespace kivo::video::source_core::test;
namespace lf = kivo::video::providers::local_file;

static std::string make_tmp(const std::string& c) {
    char buf[256]; std::snprintf(buf, sizeof(buf), "k_r_%d.tmp", std::rand());
    std::string p = buf;
    std::ofstream f(p, std::ios::binary); f.write(c.data(), static_cast<std::streamsize>(c.size())); char abs[512]; _fullpath(abs, p.c_str(), 512); return std::string(abs);
}

int main() {
    auto tmp = make_tmp("abcdef");
    lf::LocalFileSessionStore store;
    auto open_r = lf::LocalFileProvider::open(SourceOpenRequest{SourceUri::from_raw(tmp)}, store);
    CHECK_TRUE(open_r.is_success());
    auto sid = open_r.session_if_success()->session_id;
    SourceReadRequest req{sid, std::nullopt, 3};
    auto rr = lf::LocalFileProvider::read(req, store);
    CHECK_TRUE(rr.is_success());
    CHECK_EQ(rr.bytes_read, 3ULL);
    SourceReadRequest preq{sid, 4ULL, 1};
    auto prr = lf::LocalFileProvider::read(preq, store);
    CHECK_TRUE(prr.is_success());
    CHECK_EQ(prr.bytes_read, 1ULL);
    SourceReadRequest oreq{sid, 100ULL, 1};
    auto orr = lf::LocalFileProvider::read(oreq, store);
    CHECK_TRUE(orr.eof);
    std::remove(tmp.c_str());
    return 0;
}
