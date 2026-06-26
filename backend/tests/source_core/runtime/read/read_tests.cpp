#include "kivo/video/source_core/runtime/read/source_read_request.hpp"
#include "kivo/video/source_core/runtime/read/source_read_result.hpp"
#include "source_core/test_helpers.hpp"

using namespace kivo::video::source_core;
using namespace kivo::video::source_core::test;

int main() {
    SourceReadRequest req;
    req.session_id = SourceSessionId{1};
    req.offset = 0; req.length = 4096;
    CHECK_EQ(req.session_id.value, 1ULL);
    CHECK_EQ(req.offset, 0ULL);
    CHECK_EQ(req.length, 4096ULL);

    SourceReadResult result;
    result.bytes_read = 4096; result.eof = false;
    result.would_block = false; result.error = SourceError::ok();
    CHECK_TRUE(result.is_success());
    CHECK_EQ(result.bytes_read, 4096ULL);
    CHECK_TRUE(!result.eof);

    SourceReadResult wb;
    wb.would_block = true; wb.error = SourceError::ok();
    CHECK_TRUE(!wb.is_success());

    SourceReadResult eof;
    eof.eof = true; eof.error = SourceError::ok();
    CHECK_TRUE(eof.eof);
    CHECK_TRUE(eof.is_success());
    return 0;
}
