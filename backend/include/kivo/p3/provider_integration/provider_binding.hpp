#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include "kivo/p3/source_model/source_identity.hpp"
#include "kivo/p3/source_model/source_kinds.hpp"

namespace kivo::p3::provider_integration {

/// ProviderBinding is a one-shot binding from origin+provider+credential to identity+capability+plan.
/// It must NOT store global provider registry state (no provider_manager behavior).
/// It must NOT generate final PlaybackDecision.
/// schema_version: p3.provider.binding.v1
struct ProviderBinding
{
    std::string schema_version{"p3.provider.binding.v1"};
    std::string binding_id{};
    kivo::p3::source_model::SourceOriginIdentity origin{};
    kivo::p3::source_model::ProviderIdentity provider{};
    std::string credential_scope{};
    kivo::p3::source_model::SourceCapability capability_hint{};
    std::string session_plan_ref{};
    std::string probe_hint_ref{};
    std::string fast_start_hint_ref{};
    std::string decision_hint_ref{};
    std::string evidence_ref{};

    bool is_valid() const noexcept { return !binding_id.empty() && !origin.origin_id.empty(); }
    bool has_credential() const noexcept { return !credential_scope.empty(); }
};

/// ProviderMetadataHint fields let provider metadata help without replacing proof.
/// ProbeSkipPolicy only after P3 cross-validation.
struct ProviderMetadataHint
{
    std::string schema_version{"p3.provider.metadata_hint.v1"};
    std::string container_hint{};
    std::string codec_hint{};
    std::optional<int64_t> duration_ns{};
    int32_t stream_count{0};
    std::string subtitle_hint{};
    std::string hdr_hint{};
    std::string drm_hint{};
    double confidence{0.0};
    std::string source_hint{};

    bool is_high_confidence() const noexcept { return confidence >= 0.9; }
};

/// ProbeSkipPolicy defines when probe can be skipped after cross-validation.
enum class ProbeSkipPolicy
{
    never_skip,
    skip_if_cross_validated,
    skip_if_provider_metadata_only
};

} // namespace kivo::p3::provider_integration
