#include "kivo/video/source_core/runtime/timeout/source_timeout.hpp"
#include "source_core/test_helpers.hpp"
#include <limits>

using namespace kivo::video::source_core;
using namespace kivo::video::source_core::test;

int main() {
    SourceTimeout t;
    CHECK_EQ(t.millis, 0ULL);
    CHECK_TRUE(!t.is_infinite());
    t.millis = 5000;
    CHECK_EQ(t.millis, 5000ULL);
    CHECK_TRUE(!t.is_infinite());
    auto inf = SourceTimeout::infinite();
    CHECK_EQ(inf, (std::numeric_limits<std::uint64_t>::max)());
    t.millis = SourceTimeout::infinite();
    CHECK_TRUE(t.is_infinite());
    return 0;
}
