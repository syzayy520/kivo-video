#include "kivo/video/source_core/runtime/cancel/source_cancel_token.hpp"

#include <cassert>

using namespace kivo::video::source_core;

int main() {
    // Default: not cancelled
    SourceCancelToken token;
    assert(!token.is_cancelled());
    assert(!token.cancelled);
    
    // Cancel
    token.cancel();
    assert(token.is_cancelled());
    assert(token.cancelled);
    
    return 0;
}
