#include "kivo/video/source_core/runtime/error/source_session_error.hpp"
#include "source_core/test_helpers.hpp"

using namespace kivo::video::source_core;
using namespace kivo::video::source_core::test;

int main() {
    SourceSessionError err{SourceErrorCode::timeout, "timed out"};
    CHECK_TRUE(!err.is_ok());
    CHECK_EQ(err.code, SourceErrorCode::timeout);
    auto ok = SourceSessionError::ok();
    CHECK_TRUE(ok.is_ok());
    return 0;
}
