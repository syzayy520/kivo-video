#include "kivo/video/source_core/runtime/timeout/source_timeout.hpp"

#include <cassert>
#include <limits>

using namespace kivo::video::source_core;

int main() {
    // Default timeout
    SourceTimeout t;
    assert(t.millis == 0);
    assert(!t.is_infinite());
    
    // Finite timeout
    t.millis = 5000;
    assert(t.millis == 5000);
    assert(!t.is_infinite());
    
    // Infinite timeout uses max uint64
    auto inf = SourceTimeout::infinite();
    assert(inf == std::numeric_limits<std::uint64_t>::max());
    
    SourceTimeout ti;
    ti.millis = SourceTimeout::infinite();
    assert(ti.is_infinite());
    
    return 0;
}
