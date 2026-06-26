#include "kivo/video/source_core/contracts/error/source_error.hpp"

#include <cassert>

using namespace kivo::video::source_core;

int main() {
    // Default error is OK
    SourceError err;
    assert(err.is_ok());
    assert(err.code == SourceErrorCode::none);
    
    // Error with code
    SourceError not_found{SourceErrorCode::not_found, "resource missing"};
    assert(!not_found.is_ok());
    assert(not_found.code == SourceErrorCode::not_found);
    
    // Static OK utility
    auto ok = SourceError::ok();
    assert(ok.is_ok());
    
    // All error codes are distinct
    assert(SourceErrorCode::none != SourceErrorCode::not_found);
    assert(SourceErrorCode::timeout != SourceErrorCode::auth_failed);
    
    return 0;
}
