#include "kivo/video/source_core/runtime/error/source_runtime_error.hpp"

#include <cassert>

using namespace kivo::video::source_core;

int main() {
    // Runtime error is same as contract error
    SourceRuntimeError err{SourceErrorCode::timeout, "connection timed out"};
    assert(!err.is_ok());
    assert(err.code == SourceErrorCode::timeout);
    
    SourceRuntimeError ok = SourceRuntimeError::ok();
    assert(ok.is_ok());
    
    return 0;
}
