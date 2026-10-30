#pragma once

#include <cstdint>
#include <optional>
#include <string>

namespace kivo::p3::media_probe {

/// MediaProbeProcessBoundary: isolate ffprobe and probe logic.
struct MediaProbeProcessBoundary
{
    std::string schema_version{"p3.probe.boundary.v1"};
    std::string probe_id{};
    std::string input_kind{};
    std::string safe_headers_ref{};
    bool is_local_path{false};
    bool is_remote_url{false};
    bool uses_source_session_reader{false};
    int64_t timeout_ns{0};
    int64_t max_memory_bytes{0};
};

/// FastProbe: minimal low-latency probe without lying.
struct FastProbe
{
    std::string schema_version{"p3.probe.fast.v1"};
    std::string container_hint{};
    std::string video_codec_hint{};
    std::string audio_codec_hint{};
    bool has_moov{false};
    bool has_ebml{false};
    bool has_pat_pmt{false};
    bool has_flv_header{false};
    double confidence{0.0};

    bool is_high_confidence() const noexcept { return confidence >= 0.9; }
};

/// CriticalConstraintsChecklist: make confidence dependent on real constraints.
struct CriticalConstraintsChecklist
{
    std::string schema_version{"p3.probe.constraints.v1"};
    bool source_readable{false};
    bool range_accessible{false};
    bool manifest_accessible{false};
    bool auth_valid{false};
    bool known_container{false};
    bool known_delivery{false};
    bool video_path_available{false};
    bool audio_path_available{false};
    bool decoder_available{false};
    bool audio_available{false};
    bool subtitle_available{false};
    bool drm_clear_fallback{false};
    bool network_policy_satisfied{false};
    bool redaction_policy_satisfied{false};
    bool resource_budget_satisfied{false};
    bool handoff_ready{false};

    bool all_critical_blocking() const noexcept
    {
        return source_readable && range_accessible && auth_valid && known_container
            && video_path_available && audio_path_available && decoder_available
            && network_policy_satisfied && redaction_policy_satisfied
            && resource_budget_satisfied && handoff_ready;
    }
};

/// ProbeEvidence: convert probe output to Kivo contracts (not raw ffprobe JSON).
struct ProbeEvidence
{
    std::string schema_version{"p3.probe.evidence.v1"};
    std::string container{};
    std::string video_codec{};
    std::string audio_codec{};
    std::optional<std::string> subtitle_codec{};
    std::optional<std::string> hdr_format{};
    std::optional<std::string> drm_system{};
    double confidence{0.0};
    bool has_downgrade_trigger{false};
    CriticalConstraintsChecklist constraints{};
};

} // namespace kivo::p3::media_probe
