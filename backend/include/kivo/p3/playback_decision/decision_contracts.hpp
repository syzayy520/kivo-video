#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>
#include "kivo/p3/source_model/source_kinds.hpp"

namespace kivo::p3::playback_decision {

/// CandidateKind enumerates candidate types.
enum class CandidateKind
{
    direct_play,
    direct_stream,
    hls,
    dash,
    playlist,
    server_processing_required,
    not_playable,
    probe_required,
    provider_unsupported,
    credential_required,
    network_unsupported,
    retryable_network,
    unsupported_protocol
};

/// PlaybackCandidateSet: build candidate set without playback.
struct PlaybackCandidate
{
    std::string schema_version{"p3.decision.candidate.v1"};
    std::string candidate_id{};
    CandidateKind kind{CandidateKind::not_playable};
    std::string source_ref{};
    std::string delivery_ref{};
    std::string probe_ref{};
    double quality_score{0.0};
    bool requires_p4_selection{false};
    std::vector<std::string> not_claimed_fields{};
};

struct PlaybackCandidateSet
{
    std::string schema_version{"p3.decision.candidate_set.v1"};
    std::string set_id{};
    std::vector<PlaybackCandidate> candidates{};
    std::vector<std::string> not_claimed_fields{};

    bool is_empty() const noexcept { return candidates.empty(); }
    size_t size() const noexcept { return candidates.size(); }
};

/// CandidateRanking: rank candidates with policy reasons and no provider dependency.
struct CandidateRanking
{
    std::string schema_version{"p3.decision.ranking.v1"};
    std::string ranking_id{};
    std::vector<std::string> ranked_candidate_ids{};
    std::vector<std::string> reasons{};
    std::vector<std::string> rejected_candidate_ids{};
    std::vector<std::string> deferred_candidate_ids{};

    bool has_reasons() const noexcept { return !reasons.empty(); }
};

/// PairwiseCandidateComparison: explain candidate choices pairwise.
struct PairwiseCandidateComparison
{
    std::string schema_version{"p3.decision.pairwise.v1"};
    std::string comparison_id{};
    std::string winner_id{};
    std::string loser_id{};
    std::string reason{};
    std::vector<std::string> winning_factors{};
    std::vector<std::string> losing_factors{};
    std::string policy_rule{};
};

/// PlaybackDecisionTrace: make decisions auditable.
struct PlaybackDecisionTrace
{
    std::string schema_version{"p3.decision.trace.v1"};
    std::string trace_id{};
    std::string execution_card_id{};
    std::vector<std::string> input_constraints{};
    std::vector<std::string> evidence_refs{};
    std::vector<std::string> missing_evidence{};
    std::vector<std::string> ranking_factors{};
    std::vector<std::string> excluded_candidates{};
    std::vector<std::string> runtime_revision_points{};
    double confidence{0.0};
    std::vector<std::string> not_claimed_fields{};

    bool is_bidirectionally_linked() const noexcept { return !execution_card_id.empty(); }
};

/// RuntimeRevisionResult: support safe re-decision without controlling playback core.
struct RuntimeRevisionResult
{
    std::string schema_version{"p3.decision.revision.v1"};
    std::string revision_id{};
    std::string trigger{};
    std::string action{};
    std::string suggested_candidate_id{};
    bool p4_action_hint{false};
};

} // namespace kivo::p3::playback_decision
