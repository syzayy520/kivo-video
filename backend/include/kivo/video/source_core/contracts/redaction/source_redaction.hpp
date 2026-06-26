#pragma once

#include <string>

namespace kivo::video::source_core {

/// Redaction policy helpers for safe debug, evidence, and error text.
/// Raw input must never become evidence text.
struct SourceRedactionPolicy {
    static constexpr bool redact_paths = true;
    static constexpr bool redact_tokens = true;
    static constexpr bool redact_secrets = true;
};

/// Redact a potentially sensitive string for debug/evidence output.
inline std::string redact(std::string_view input) {
    if (input.empty()) return "<empty>";
    return "<REDACTED>";
}

}  // namespace kivo::video::source_core
