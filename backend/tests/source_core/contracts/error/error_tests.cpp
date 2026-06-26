#include "kivo/video/source_core/contracts/error/source_error.hpp"
#include "source_core/test_helpers.hpp"

using namespace kivo::video::source_core;
using namespace kivo::video::source_core::test;

int main() {
    SourceError err;
    CHECK_TRUE(err.is_ok());
    CHECK_EQ(err.code, SourceErrorCode::none);

    SourceError nf{SourceErrorCode::not_found, "resource missing"};
    CHECK_TRUE(!nf.is_ok());
    CHECK_EQ(nf.code, SourceErrorCode::not_found);

    auto ok = SourceError::ok();
    CHECK_TRUE(ok.is_ok());
    CHECK_NEQ(SourceErrorCode::none, SourceErrorCode::not_found);
    return 0;
}
