#include "kivo/video/source_core/contracts/redaction/source_redaction.hpp"
#include "source_core/test_helpers.hpp"

#include <string>
using namespace kivo::video::source_core;
using namespace kivo::video::source_core::test;

int main() {
    CHECK_EQ(redact("sensitive_data"), std::string("<REDACTED>"));
    CHECK_EQ(redact(""), std::string("<empty>"));
    CHECK_TRUE(SourceRedactionPolicy::redact_paths);
    CHECK_TRUE(SourceRedactionPolicy::redact_tokens);
    CHECK_TRUE(SourceRedactionPolicy::redact_secrets);
    return 0;
}
