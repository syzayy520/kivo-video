#include "kivo/video/source_core/runtime/session/source_session.hpp"
#include "source_core/test_helpers.hpp"
#include <string>

using namespace kivo::video::source_core;
using namespace kivo::video::source_core::test;

int main() {
    SourceSession s;
    CHECK_EQ(s.session_id.value, 0ULL);
    CHECK_EQ(s.session_state, SourceSessionState::created);
    CHECK_EQ(s.contract_version, kSourceCoreContractVersion);
    s.session_state = SourceSessionState::open;
    CHECK_EQ(s.session_state, SourceSessionState::open);
    s.session_state = SourceSessionState::closed;
    CHECK_EQ(s.session_state, SourceSessionState::closed);
    s.session_state = SourceSessionState::error;
    CHECK_EQ(s.session_state, SourceSessionState::error);
    s.session_id = SourceSessionId{7};
    std::string dbg = s.debug_string();
    CHECK_TRUE(dbg.find("7") != std::string::npos);
    CHECK_TRUE(dbg.find("3") != std::string::npos);
    SourceSessionId x{1}, y{1};
    CHECK_EQ(x, y);
    return 0;
}
