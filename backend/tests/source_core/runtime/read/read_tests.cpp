#include "kivo/video/source_core/runtime/read/source_read_request.hpp"
#include "kivo/video/source_core/runtime/read/source_read_result.hpp"

#include <cassert>

using namespace kivo::video::source_core;

int main() {
    // Read request bound to SourceSessionId
    SourceReadRequest req;
    req.session_id = SourceSessionId{1};
    req.offset = 0;
    req.length = 4096;
    
    assert(req.session_id.value == 1);
    assert(req.offset == 0);
    assert(req.length == 4096);
    
    // Read result: success
    SourceReadResult result;
    result.bytes_read = 4096;
    result.eof = false;
    result.would_block = false;
    result.error = SourceError::ok();
    assert(result.is_success());
    assert(result.bytes_read == 4096);
    assert(!result.eof);
    
    // Read result: would-block
    SourceReadResult wb;
    wb.would_block = true;
    wb.error = SourceError::ok();
    assert(!wb.is_success());  // would-block means not success
    
    // Read result: EOF
    SourceReadResult eof_result;
    eof_result.eof = true;
    eof_result.bytes_read = 0;
    eof_result.error = SourceError::ok();
    assert(eof_result.eof);
    assert(eof_result.is_success());
    
    return 0;
}
