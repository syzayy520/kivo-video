#include "kivo/video/providers/local_file/local_file_provider.hpp"
#include "source_core/test_helpers.hpp"
#include <fstream>
#include <cstdio>
#include <cstdlib>

using namespace kivo::video::source_core;
using namespace kivo::video::source_core::test;
namespace lf = kivo::video::providers::local_file;

int main() {
    char b[256]; std::snprintf(b,sizeof(b),"k_rdt_%d.tmp",std::rand());
    std::string tmp; {char abs[512]; _fullpath(abs,b,512); tmp=abs;} {std::ofstream f(tmp); f<<"hello";}
    lf::LocalFileSessionStore store;
    auto open_r = lf::LocalFileProvider::open(SourceOpenRequest{SourceUri::from_raw(tmp)}, store);
    CHECK_TRUE(open_r.is_success());
    auto ev = lf::LocalFileProvider::get_evidence(open_r.session_if_success()->session_id, store);
    CHECK_TRUE(ev.uri_redacted.find(tmp) == std::string::npos);
    CHECK_TRUE(ev.uri_redacted.find("sha256-12") != std::string::npos);
    std::remove(tmp.c_str());
    return 0;
}
