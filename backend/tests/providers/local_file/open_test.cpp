#include "kivo/video/providers/local_file/local_file_provider.hpp"
#include "source_core/test_helpers.hpp"
#include <fstream>
#include <cstdio>
#include <cstdlib>

using namespace kivo::video::source_core;
using namespace kivo::video::source_core::test;
namespace lf = kivo::video::providers::local_file;

static std::string make_tmp(const std::string& content) {
    char buf[256];
    std::snprintf(buf, sizeof(buf), "kivo_lf_test_%d.tmp", std::rand());
    std::string p = buf;
    std::ofstream f(p); f << content;
    char abs[512]; _fullpath(abs, p.c_str(), 512); return std::string(abs);
}

int main() {
    auto tmp = make_tmp("hello");
    lf::LocalFileSessionStore store;
    auto result = lf::LocalFileProvider::open(SourceOpenRequest{SourceUri::from_raw(tmp)}, store);
    CHECK_TRUE(result.is_success());
    CHECK_EQ(result.session_if_success()->session_state, SourceSessionState::open);
    CHECK_EQ(result.session_if_success()->provider_kind, ProviderKind::local_file);
    auto r2 = lf::LocalFileProvider::open(SourceOpenRequest{SourceUri::from_raw("")}, store);
    CHECK_TRUE(r2.is_failure());
    auto r3 = lf::LocalFileProvider::open(SourceOpenRequest{SourceUri::from_raw("C:\\no_such_file.xyz")}, store);
    CHECK_TRUE(r3.is_failure());
    std::remove(tmp.c_str());
    return 0;
}
