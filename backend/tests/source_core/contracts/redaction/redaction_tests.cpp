#include "kivo/video/source_core/contracts/redaction/source_redaction.hpp"

#include <cassert>
#include <string>

using namespace kivo::video::source_core;

int main() {
    // Redact non-empty string
    std::string result = redact("sensitive_data");
    assert(result == "<REDACTED>");
    
    // Redact empty string
    std::string empty_result = redact("");
    assert(empty_result == "<empty>");
    
    // Policy constants are true
    static_assert(SourceRedactionPolicy::redact_paths);
    static_assert(SourceRedactionPolicy::redact_tokens);
    static_assert(SourceRedactionPolicy::redact_secrets);
    
    return 0;
}
