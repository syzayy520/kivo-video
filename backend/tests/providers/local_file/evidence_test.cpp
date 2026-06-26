#include "kivo/video/providers/local_file/local_file_provider.hpp"
#include "source_core/test_helpers.hpp"
#include <fstream>
#include <cstdio>
#include <cstdlib>

using namespace kivo::video::source_core;
using namespace kivo::video::source_core::test;
namespace lf = kivo::video::providers::local_file;

int main() {
    char b[256]; std::snprintf(b,sizeof(b),"k_e_%d.tmp",std::rand());
    std::string tmp; {char abs[512]; _fullpath(abs,b,512); tmp=abs;} {std::ofstream f(tmp); f<<"test";}
    lf::LocalFileSessionStore store;
    auto open_r = lf::LocalFileProvider::open(SourceOpenRequest{SourceUri::from_raw(tmp)}, store);
    CHECK_TRUE(open_r.is_success());
    auto ev = lf::LocalFileProvider::get_evidence(open_r.session_if_success()->session_id, store);
    CHECK_TRUE(ev.has_runtime_pass());
    CHECK_TRUE(!ev.is_contract_pass_only());
    CHECK_EQ(ev.kind, ProviderKind::local_file);
    for (auto& item : ev.items) {
        CHECK_TRUE(item.safe_description.find(tmp) == std::string::npos);
    }
    std::remove(tmp.c_str());
    return 0;
}
