#pragma once

#include <string>
#include <string_view>

namespace kivo::video::source_core {

/// Raw source URI -- exists only at the open-request boundary.
/// No public raw-value accessor.
/// Debug output must be redacted.
class SourceUri {
public:
    static SourceUri from_raw(std::string raw) {
        SourceUri u;
        u.raw_ = std::move(raw);
        return u;
    }

    std::string redacted_debug() const {
        if (raw_.empty()) return "<empty>";
        // Redact: show scheme only if present
        auto pos = raw_.find("://");
        if (pos != std::string::npos) {
            return raw_.substr(0, pos + 3) + "<REDACTED>";
        }
        return "<REDACTED>";
    }

    bool empty() const { return raw_.empty(); }

private:
    std::string raw_;
    friend class CanonicalSourceUri;
    friend class RedactedSourceUri;
};

}  // namespace kivo::video::source_core
