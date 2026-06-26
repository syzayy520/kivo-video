#pragma once

#include "kivo/video/source_core/contracts/uri/redacted_source_uri.hpp"

#include <string>
#include <cstdint>

namespace kivo::video::providers::local_file {

/// Build a redacted URI from the opaque digest prefix + extension.
source_core::RedactedSourceUri make_redacted_uri(
    const std::string& digest_prefix,
    const std::string& extension);

}  // namespace kivo::video::providers::local_file
