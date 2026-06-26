#include "kivo/video/source_core/runtime/seek/source_seek_request.hpp"
#include "kivo/video/source_core/runtime/seek/source_seek_result.hpp"
#include "source_core/test_helpers.hpp"

using namespace kivo::video::source_core;
using namespace kivo::video::source_core::test;

int main() {
    SourceSeekRequest req;
    req.session_id = SourceSessionId{1};
    req.offset = 100; req.origin = SeekOrigin::begin;
    CHECK_EQ(req.session_id.value, 1ULL);
    CHECK_EQ(req.offset, 100LL);
    CHECK_EQ(req.origin, SeekOrigin::begin);

    SourceSeekResult result;
    result.new_position = 100; result.error = SourceError::ok();
    CHECK_TRUE(result.is_success());
    CHECK_TRUE(result.new_position.has_value());
    CHECK_EQ(result.new_position.value(), 100ULL);

    SourceSeekResult err_result;
    err_result.error = SourceError{SourceErrorCode::range_not_supported, ""};
    err_result.new_position = std::nullopt;
    CHECK_TRUE(!err_result.is_success());
    CHECK_TRUE(!err_result.new_position.has_value());
    CHECK_NEQ(SeekOrigin::begin, SeekOrigin::end);
    return 0;
}
