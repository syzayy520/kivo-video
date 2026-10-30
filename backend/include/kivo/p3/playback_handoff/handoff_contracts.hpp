#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace kivo::p3::playback_handoff {

/// HandoffOwnershipModel: constrain ownership to three models.
enum class HandoffOwnershipModel
{
    transfer_ownership,
    borrowed_until_close,
    reopen_by_plan
};

/// PlaybackHandoffContract: prepare P4 handoff without playing.
struct PlaybackHandoffContract
{
    std::string schema_version{"p3.handoff.contract.v1"};
    std::string handoff_id{};
    std::string candidate_set_ref{};
    std::optional<std::string> selected_candidate_ref{};
    std::string session_lease_ref{};
    HandoffOwnershipModel ownership{HandoffOwnershipModel::transfer_ownership};
    std::string close_responsibility{};
    std::string cancellation_bridge{};
    std::string deadline_bridge{};
    std::string session_error_bridge{};
    std::string hardware_constraints_ref{};
    std::string buffer_requirement_ref{};
    std::string runtime_revision_policy{};
    std::string evidence_ref{};
    std::string state{};

    bool has_selected_candidate() const noexcept { return selected_candidate_ref.has_value(); }
};

/// PlaybackDecisionRuntime: runtime decision state.
struct PlaybackDecisionRuntime
{
    std::string schema_version{"p3.handoff.runtime.v1"};
    std::string runtime_id{};
    std::string handoff_ref{};
    std::string current_state{};
    bool is_active{false};
};

/// FailureTaxonomy: normalize failure classes.
enum class FailureClass
{
    binding_failure,
    transport_failure,
    probe_failure,
    capability_failure,
    handoff_failure,
    runtime_revision_failure,
    manifest_failure,
    playlist_failure,
    session_failure,
    security_failure
};

struct FailureTaxonomy
{
    std::string schema_version{"p3.handoff.failure_taxonomy.v1"};
    FailureClass failure_class{FailureClass::session_failure};
    std::string detailed_reason{};
    bool is_recoverable{false};
    bool is_retryable{false};
};

} // namespace kivo::p3::playback_handoff
