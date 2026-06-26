#include "kivo/video/providers/local_file/local_file_provider.hpp"
#include "source_core/test_helpers.hpp"
#include <fstream>
#include <cstdio>
#include <cstdlib>

using namespace kivo::video::source_core;
using namespace kivo::video::source_core::test;
namespace lf = kivo::video::providers::local_file;

static std::string mk(const std::string& c) {
    char b[256]; std::snprintf(b,sizeof(b),"k_s_%d.tmp",std::rand());
    char abs[512]; _fullpath(abs,b,512);
    std::string p=abs;
    std::ofstream f(p,std::ios::binary);
    f.write(c.data(),static_cast<std::streamsize>(c.size())); return p;
}

int main() {
    auto tmp = mk("abcdef");
    lf::LocalFileSessionStore store;
    auto open_r = lf::LocalFileProvider::open(SourceOpenRequest{SourceUri::from_raw(tmp)}, store);
    CHECK_TRUE(open_r.is_success());
    auto sid = open_r.session_if_success()->session_id;
    SourceSeekRequest sreq{sid, 2, SeekOrigin::begin};
    auto sr = lf::LocalFileProvider::seek(sreq, store);
    CHECK_TRUE(sr.is_success());
    CHECK_EQ(sr.new_position.value(), 2ULL);
    SourceSeekRequest ereq{sid, -2, SeekOrigin::end};
    auto esr = lf::LocalFileProvider::seek(ereq, store);
    CHECK_TRUE(esr.is_success());
    CHECK_EQ(esr.new_position.value(), 4ULL);
    SourceSeekRequest breq{sid, 100, SeekOrigin::begin};
    CHECK_TRUE(!lf::LocalFileProvider::seek(breq, store).is_success());
    std::remove(tmp.c_str());
    return 0;
}
