#pragma once

#include "kivo/video/source_core/contracts/uri/canonical_source_uri.hpp"

#include <string>

namespace kivo::video::source_core {

/// Safe-for-evidence URI. No raw paths, no tokens, no secrets.
class RedactedSourceUri {
public:
    static RedactedSourceUri from_canonical(const CanonicalSourceUri& canonical) {
        RedactedSourceUri r;
        r.display_ = canonical.to_string();
        return r;
    }

    static RedactedSourceUri from_raw(const std::string& already_redacted) {
        RedactedSourceUri r;
        r.display_ = already_redacted;
        return r;
    }

    std::string display() const { return display_; }

private:
    std::string display_;
};

}  // namespace kivo::video::source_core
