#pragma once

#include <array>
#include <cstdint>

namespace kivo::video::playback_graph::runtime {

struct PrivacyDiagnosticString {
    std::array<char, 64> text{};
};

class PrivacySaltLifecycle {
public:
    void generate_for_device(std::uint64_t seed) noexcept;
    void rotate(std::uint64_t seed) noexcept;

    [[nodiscard]] std::uint64_t salt_id() const noexcept;
    [[nodiscard]] PrivacyDiagnosticString redact_source_id(std::uint64_t source_id) const noexcept;
    [[nodiscard]] PrivacyDiagnosticString crash_report_tag() const noexcept;
    [[nodiscard]] bool crash_report_contains_raw_salt() const noexcept;

private:
    std::array<std::uint8_t, 16> raw_salt_{};
    std::uint64_t salt_id_{0};
};

}  // namespace kivo::video::playback_graph::runtime
