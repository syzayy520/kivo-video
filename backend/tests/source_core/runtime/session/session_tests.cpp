#include "kivo/video/source_core/runtime/session/source_session.hpp"

#include <cassert>
#include <string>

using namespace kivo::video::source_core;

int main() {
    // Default session
    SourceSession session;
    assert(session.id.value == 0);
    assert(session.state == SourceSessionState::created);
    assert(session.contract_version == kSourceCoreContractVersion);
    
    // Session state transitions
    session.state = SourceSessionState::open;
    assert(session.state == SourceSessionState::open);
    
    session.state = SourceSessionState::closed;
    assert(session.state == SourceSessionState::closed);
    
    session.state = SourceSessionState::error;
    assert(session.state == SourceSessionState::error);
    
    // Safe debug output
    session.id = SourceSessionId{7};
    std::string dbg = session.safe_debug();
    assert(dbg.find("7") != std::string::npos);
    assert(dbg.find("3") != std::string::npos);  // state=error has value 3
    
    // SourceSessionId comparison
    SourceSessionId x{1};
    SourceSessionId y{1};
    assert(x == y);
    
    return 0;
}
