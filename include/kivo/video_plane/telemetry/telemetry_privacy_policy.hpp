// telemetry_privacy_policy.hpp — GATE-028: P5A telemetry privacy policy
#pragma once
#include <cstdint>
#include <string>

namespace kivo::video_plane::telemetry {

struct TelemetryPrivacyPolicy {
    bool collect_usage_data{false};         // collect usage telemetry
    bool collect_crash_data{true};          // collect crash data
    bool anonymize_user_data{true};         // anonymize user data
    bool require_user_consent{true};        // require explicit user consent
    std::string data_retention_policy;      // data retention description
    bool operator==(TelemetryPrivacyPolicy const&) const = default;
};

}  // namespace kivo::video_plane::telemetry
