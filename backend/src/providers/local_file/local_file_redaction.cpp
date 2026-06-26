#include "kivo/video/providers/local_file/local_file_redaction.hpp"

namespace kivo::video::providers::local_file {

source_core::RedactedSourceUri make_redacted_uri(
    const std::string& digest_prefix,
    const std::string& extension) {
    source_core::RedactedSourceUri uri;
    // Use local-file://sha256-12:<digest-prefix>.<ext> pattern
    std::string display = "local-file://sha256-12:" + digest_prefix;
    if (!extension.empty()) display += extension;
    return source_core::RedactedSourceUri::from_raw(display);
}

}  // namespace kivo::video::providers::local_file
