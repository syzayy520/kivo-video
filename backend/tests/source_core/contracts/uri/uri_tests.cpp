#include "kivo/video/source_core/contracts/uri/source_uri.hpp"
#include "kivo/video/source_core/contracts/uri/canonical_source_uri.hpp"
#include "kivo/video/source_core/contracts/uri/redacted_source_uri.hpp"

#include <cassert>
#include <string>

using namespace kivo::video::source_core;

int main() {
    // SourceUri: no public raw-value accessor
    auto su = SourceUri::from_raw("https://example.com/path?token=secret");
    assert(!su.empty());
    
    // Debug output must be redacted
    std::string dbg = su.redacted_debug();
    assert(dbg.find("secret") == std::string::npos);
    assert(dbg.find("REDACTED") != std::string::npos);
    
    // CanonicalSourceUri from raw
    auto canonical = CanonicalSourceUri::from_source(su, ProviderKind::http_file);
    assert(!canonical.to_string().empty());
    assert(canonical.provider_kind() == ProviderKind::http_file);
    
    // RedactedSourceUri from canonical
    auto redacted = RedactedSourceUri::from_canonical(canonical);
    assert(!redacted.display().empty());
    
    // Empty URI
    auto empty = SourceUri::from_raw("");
    assert(empty.empty());
    auto empty_dbg = empty.redacted_debug();
    assert(empty_dbg.find("empty") != std::string::npos);
    
    return 0;
}
