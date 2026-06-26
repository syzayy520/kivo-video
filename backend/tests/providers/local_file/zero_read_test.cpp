#include "kivo/video/providers/local_file/local_file_provider.hpp"
#include "source_core/test_helpers.hpp"
#include <fstream>
#include <cstdio>
#include <cstdlib>

using namespace kivo::video::source_core;
using namespace kivo::video::source_core::test;
namespace lf = kivo::video::providers::local_file;

int main() {
    char b[256]; std::snprintf(b,sizeof(b),"k_z_%d.tmp",std::rand());
    std::string tmp; {char abs[512]; _fullpath(abs,b,512); tmp=abs;} {std::ofstream f(tmp); f<<"test";}
    lf::LocalFileSessionStore store;
    auto open_r = lf::LocalFileProvider::open(SourceOpenRequest{SourceUri::from_raw(tmp)}, store);
    CHECK_TRUE(open_r.is_success());
    SourceReadRequest req{open_r.session_if_success()->session_id, std::nullopt, 0};
    auto rr = lf::LocalFileProvider::read(req, store);
    CHECK_TRUE(rr.is_success());
    CHECK_EQ(rr.bytes_read, 0ULL);
    std::remove(tmp.c_str());
    return 0;
}
