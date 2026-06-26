#include "kivo/video/providers/local_file/local_file_provider.hpp"
#include "source_core/test_helpers.hpp"
#include <fstream>
#include <cstdio>
#include <cstdlib>

using namespace kivo::video::source_core;
using namespace kivo::video::source_core::test;
namespace lf = kivo::video::providers::local_file;

int main() {
    char b[256]; std::snprintf(b,sizeof(b),"k_di_%d.tmp",std::rand());
    char abs[512]; _fullpath(abs,b,512); std::string tmp=abs; {std::ofstream f(tmp); f<<"test";}
    lf::LocalFileSessionStore store;
    auto orr = lf::LocalFileProvider::open(SourceOpenRequest{SourceUri::from_raw(tmp)}, store);
    CHECK_TRUE(orr.is_success());
    auto sid = orr.session_if_success()->session_id;

    // DirectPlayInput with access_ref
    auto dpi_r = lf::LocalFileProvider::make_direct_play_input(sid, store);
    CHECK_TRUE(dpi_r.is_success());
    auto old_ref = dpi_r.input->access_ref;
    CHECK_EQ(old_ref.session_id.value, sid.value);

    // Resolve access_ref while session open
    auto res_err = lf::LocalFileProvider::resolve_access_ref(old_ref, store);
    CHECK_TRUE(res_err.is_ok());

    // Close session
    lf::LocalFileProvider::close(sid, store);

    // Resolve OLD access_ref after close → stale_reference
    auto res2 = lf::LocalFileProvider::resolve_access_ref(old_ref, store);
    CHECK_TRUE(!res2.is_ok());
    CHECK_EQ(res2.code, SourceErrorCode::stale_reference);

    // DirectInput after close returns session_closed
    auto dpi_after = lf::LocalFileProvider::make_direct_play_input(sid, store);
    CHECK_TRUE(!dpi_after.is_success());
    CHECK_TRUE(dpi_after.error.has_value());
    CHECK_EQ(dpi_after.error->code, SourceErrorCode::session_closed);

    // Unknown session
    auto dpi_unknown = lf::LocalFileProvider::make_direct_play_input(SourceSessionId{99999}, store);
    CHECK_TRUE(!dpi_unknown.is_success());

    std::remove(tmp.c_str());
    return 0;
}
