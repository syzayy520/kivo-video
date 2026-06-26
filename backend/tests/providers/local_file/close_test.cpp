#include "kivo/video/providers/local_file/local_file_provider.hpp"
#include "source_core/test_helpers.hpp"
#include <fstream>
#include <cstdio>
#include <cstdlib>
#include <iostream>

using namespace kivo::video::source_core;
using namespace kivo::video::source_core::test;
namespace lf = kivo::video::providers::local_file;

int main() {
    char b[256]; std::snprintf(b,sizeof(b),"k_c_%d.tmp",std::rand());
    char abs[512]; _fullpath(abs,b,512);
    std::string tmp=abs; {std::ofstream f(tmp); f<<"test";}
    lf::LocalFileSessionStore store;
    auto open_r = lf::LocalFileProvider::open(SourceOpenRequest{SourceUri::from_raw(tmp)}, store);
    CHECK_TRUE(open_r.is_success());
    auto sid = open_r.session_if_success()->session_id;
    std::cerr << "session_id.value=" << sid.value << std::endl;
    auto cerr = lf::LocalFileProvider::close(sid, store);
    if (!cerr.is_ok()) {
        std::cerr << "close error: code=" << static_cast<int>(cerr.code) << " msg=" << cerr.message << std::endl;
    }
    CHECK_TRUE(cerr.is_ok());
    auto cerr2 = lf::LocalFileProvider::close(sid, store);
    CHECK_TRUE(cerr2.is_ok());
    SourceReadRequest req{sid, std::nullopt, 1};
    CHECK_TRUE(!lf::LocalFileProvider::read(req, store).is_success());
    std::remove(tmp.c_str());
    return 0;
}
