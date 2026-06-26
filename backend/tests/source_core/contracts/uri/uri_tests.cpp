#include "kivo/video/source_core/contracts/uri/source_uri.hpp"
#include "kivo/video/source_core/contracts/uri/canonical_source_uri.hpp"
#include "kivo/video/source_core/contracts/uri/redacted_source_uri.hpp"
#include "source_core/test_helpers.hpp"

#include <string>

using namespace kivo::video::source_core;
using namespace kivo::video::source_core::test;

int main() {
    auto su = SourceUri::from_raw("https://example.com/path?token=secret");
    CHECK_TRUE(!su.empty());
    std::string dbg = su.redacted_debug();
    CHECK_TRUE(dbg.find("secret") == std::string::npos);
    CHECK_TRUE(dbg.find("REDACTED") != std::string::npos);

    auto canonical = CanonicalSourceUri::from_source(su, ProviderKind::http_file);
    CHECK_TRUE(!canonical.to_string().empty());
    CHECK_EQ(canonical.provider_kind(), ProviderKind::http_file);

    auto redacted = RedactedSourceUri::from_canonical(canonical);
    CHECK_TRUE(!redacted.display().empty());

    auto empty = SourceUri::from_raw("");
    CHECK_TRUE(empty.empty());
    CHECK_TRUE(empty.redacted_debug().find("empty") != std::string::npos);
    return 0;
}
