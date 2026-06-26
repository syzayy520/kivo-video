#pragma once

#include "kivo/video/source_core/contracts/provider_kind/provider_kind.hpp"
#include "kivo/video/source_core/contracts/uri/redacted_source_uri.hpp"

#include <cstdint>
#include <string>

namespace kivo::video::source_core {

struct SourceIdentityId {
    std::uint64_t value{0};
    bool operator==(const SourceIdentityId& other) const { return value == other.value; }
};

struct SourceIdentity {
    SourceIdentityId id;
    ProviderKind kind{ProviderKind::unknown};
    RedactedSourceUri display_uri;
    std::string safe_label;  // safe for logs/evidence, no raw input

    // No raw URI, no local path, no credentials, no platform objects.
};

}  // namespace kivo::video::source_core
