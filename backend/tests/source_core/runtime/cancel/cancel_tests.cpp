#include "kivo/video/source_core/runtime/cancel/source_cancel_token.hpp"
#include "source_core/test_helpers.hpp"

using namespace kivo::video::source_core;
using namespace kivo::video::source_core::test;

int main() {
    SourceCancelToken token;
    CHECK_TRUE(!token.is_cancelled());
    CHECK_TRUE(!token.cancelled);
    token.cancel();
    CHECK_TRUE(token.is_cancelled());
    CHECK_TRUE(token.cancelled);
    return 0;
}
