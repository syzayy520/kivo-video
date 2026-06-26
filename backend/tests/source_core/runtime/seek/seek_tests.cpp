#include "kivo/video/source_core/runtime/seek/source_seek_request.hpp"
#include "kivo/video/source_core/runtime/seek/source_seek_result.hpp"

#include <cassert>

using namespace kivo::video::source_core;

int main() {
    // Seek request bound to SourceSessionId
    SourceSeekRequest req;
    req.session_id = SourceSessionId{1};
    req.offset = 100;
    req.origin = SeekOrigin::begin;
    
    assert(req.session_id.value == 1);
    assert(req.offset == 100);
    assert(req.origin == SeekOrigin::begin);
    
    // Seek result: success
    SourceSeekResult result;
    result.new_position = 100;
    result.error = SourceError::ok();
    assert(result.is_success());
    assert(result.new_position.has_value());
    assert(result.new_position.value() == 100);
    
    // Seek result: new_position is undefined (nullopt) when error exists
    SourceSeekResult err_result;
    err_result.error = SourceError{SourceErrorCode::range_not_supported, ""};
    err_result.new_position = std::nullopt;
    assert(!err_result.is_success());
    assert(!err_result.new_position.has_value());
    
    // SeekOrigin values
    assert(SeekOrigin::begin != SeekOrigin::end);
    assert(SeekOrigin::current != SeekOrigin::begin);
    
    return 0;
}
