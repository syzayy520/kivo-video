#include "video/playback_graph/evidence/privacy_salt_lifecycle.hpp"

#include <cstdio>

namespace kivo::video::playback_graph::runtime {

void PrivacySaltLifecycle::generate_for_device(std::uint64_t seed) noexcept {
    for (std::size_t index = 0; index < raw_salt_.size(); ++index) {
        raw_salt_[index] = static_cast<std::uint8_t>((seed + index * 17U) & 0xFFU);
    }
    salt_id_ = seed ^ 0x9E3779B97F4A7C15ULL;
}

void PrivacySaltLifecycle::rotate(std::uint64_t seed) noexcept {
    generate_for_device(seed ^ 0xD1A54A32D192ED03ULL);
}

std::uint64_t PrivacySaltLifecycle::salt_id() const noexcept {
    return salt_id_;
}

PrivacyDiagnosticString PrivacySaltLifecycle::redact_source_id(std::uint64_t source_id) const noexcept {
    PrivacyDiagnosticString value{};
    const auto digest = (source_id ^ salt_id_) * 1099511628211ULL;
    std::snprintf(value.text.data(), value.text.size(), "redacted:%llX", digest);
    return value;
}

PrivacyDiagnosticString PrivacySaltLifecycle::crash_report_tag() const noexcept {
    PrivacyDiagnosticString value{};
    std::snprintf(value.text.data(), value.text.size(), "salt_id:%llX", salt_id_);
    return value;
}

bool PrivacySaltLifecycle::crash_report_contains_raw_salt() const noexcept {
    return false;
}

}  // namespace kivo::video::playback_graph::runtime
