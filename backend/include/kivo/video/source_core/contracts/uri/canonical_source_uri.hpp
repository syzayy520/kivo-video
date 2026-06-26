#pragma once

#include "kivo/video/source_core/contracts/provider_kind/provider_kind.hpp"
#include "kivo/video/source_core/contracts/uri/source_uri.hpp"

#include <string>

namespace kivo::video::source_core {

/// Normalized, canonical URI derived from raw input.
/// Never contains tokens, query secrets, or user paths.
class CanonicalSourceUri {
public:
    static CanonicalSourceUri from_source(const SourceUri& raw, ProviderKind kind) {
        CanonicalSourceUri c;
        c.canonical_ = raw.redacted_debug();  // skeleton: use redacted form
        c.kind_ = kind;
        return c;
    }

    std::string to_string() const { return canonical_; }
    ProviderKind provider_kind() const { return kind_; }

private:
    std::string canonical_;
    ProviderKind kind_{ProviderKind::unknown};
};

}  // namespace kivo::video::source_core
