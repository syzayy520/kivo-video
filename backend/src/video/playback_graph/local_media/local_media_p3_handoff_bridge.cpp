#include "video/playback_graph/local_media/local_media_p3_handoff_bridge.hpp"

#include <algorithm>
#include <cstring>

namespace kivo::video::playback_graph::local_media {
namespace {

void copy_text(char* const destination, const std::size_t capacity, const std::string& source) noexcept {
    if (destination == nullptr || capacity == 0) {
        return;
    }
    const std::size_t count = std::min(capacity - 1, source.size());
    if (count > 0) {
        std::memcpy(destination, source.data(), count);
    }
    destination[count] = '\0';
}

[[nodiscard]] std::string make_id(const char* const prefix, const std::uint64_t source_id) {
    return std::string(prefix) + std::to_string(source_id);
}

}  // namespace

bool LocalMediaP3HandoffBridge::build(
    const std::uint64_t source_id,
    const kivo::video::video_plane::runtime::local_media::LocalMediaPipelineStatus&
        status) noexcept {
    reset();
    if (!status.source_session_connected) {
        return false;
    }

    try {
        source_ref_ = make_id("p2-source-session:", source_id);
        selected_candidate_ref_ = make_id("p3-direct-play-candidate:", source_id);

        selected_candidate_.candidate_id = selected_candidate_ref_;
        selected_candidate_.kind = kivo::p3::playback_decision::CandidateKind::direct_play;
        selected_candidate_.source_ref = source_ref_;
        selected_candidate_.delivery_ref = status.source_provider_kind;
        selected_candidate_.probe_ref = make_id("p3-local-probe:", source_id);
        selected_candidate_.quality_score = 1.0;
        selected_candidate_.requires_p4_selection = true;
        selected_candidate_.not_claimed_fields = {
            "no_remote_auth_reconnect_claim",
            "no_manifest_runtime_claim",
            "no_transcode_claim",
        };

        candidate_set_.set_id = make_id("p3-local-candidate-set:", source_id);
        candidate_set_.candidates = {selected_candidate_};
        candidate_set_.not_claimed_fields = {
            "single_local_direct_play_candidate_only",
        };

        handoff_.handoff_id = make_id("p3-local-handoff:", source_id);
        handoff_.candidate_set_ref = candidate_set_.set_id;
        handoff_.selected_candidate_ref = selected_candidate_.candidate_id;
        handoff_.session_lease_ref = source_ref_;
        handoff_.ownership =
            kivo::p3::playback_handoff::HandoffOwnershipModel::borrowed_until_close;
        handoff_.close_responsibility = "p7_local_media_close";
        handoff_.cancellation_bridge = "p7_command_generation";
        handoff_.deadline_bridge = "p7_local_open_deadline";
        handoff_.session_error_bridge = "p7_recovery_coordinator";
        handoff_.buffer_requirement_ref = "p3_local_direct_play_buffer_requirement";
        handoff_.runtime_revision_policy = "p4_reads_p3_runtime_revision_view";
        handoff_.evidence_ref = selected_candidate_.probe_ref;
        handoff_.state = "ready_for_p4_open";

        p3_source_session_ref_.ref = source_ref_.data();
        p3_candidate_set_ref_.ref = &candidate_set_;
        p3_selected_candidate_ref_.ref = &selected_candidate_;
        p3_probe_evidence_.has_duration = status.duration_ms > 0;
        p3_probe_evidence_.duration_ms =
            kivo::playback::control_plane::primitives::Milliseconds{
                status.duration_ms > 0 ? static_cast<std::uint64_t>(status.duration_ms) : 0U};
        p3_probe_evidence_.is_seekable = status.duration_ms > 0;
        p3_probe_evidence_.has_reliable_byte_to_time_mapping = false;
        p3_probe_evidence_.is_live_like = false;

        p4_open_envelope_.session_id =
            kivo::playback::control_plane::revision::SessionId{source_id};
        p4_open_envelope_.source_session = p3_source_session_ref_;
        p4_open_envelope_.candidate_set = p3_candidate_set_ref_;
        p4_open_envelope_.selected_candidate = p3_selected_candidate_ref_;
        p4_open_envelope_.probe_evidence = &p3_probe_evidence_;
        p4_open_envelope_.handoff_evidence.import_successful = true;
        p4_open_envelope_.handoff_evidence.conflict_notes.clear();

        status_.candidate_set_created = !candidate_set_.is_empty();
        status_.handoff_created = handoff_.has_selected_candidate();
        status_.p4_open_envelope_created =
            p4_open_envelope_.handoff_evidence.import_successful &&
            p4_open_envelope_.candidate_set.ref != nullptr &&
            p4_open_envelope_.selected_candidate.ref != nullptr;
        copy_text(status_.candidate_kind,
                  sizeof(status_.candidate_kind),
                  "direct_play");
        copy_text(status_.handoff_id, sizeof(status_.handoff_id), handoff_.handoff_id);
        copy_text(status_.candidate_set_id,
                  sizeof(status_.candidate_set_id),
                  candidate_set_.set_id);
        return status_.p4_open_envelope_created;
    } catch (...) {
        reset();
        return false;
    }
}

void LocalMediaP3HandoffBridge::mark_consumed_by_p7() noexcept {
    if (status_.p4_open_envelope_created) {
        status_.consumed_by_p7 = true;
    }
}

void LocalMediaP3HandoffBridge::reset() noexcept {
    status_ = {};
    source_ref_.clear();
    selected_candidate_ref_.clear();
    selected_candidate_ = {};
    candidate_set_ = {};
    handoff_ = {};
    p3_source_session_ref_ = {};
    p3_candidate_set_ref_ = {};
    p3_selected_candidate_ref_ = {};
    p3_probe_evidence_ = {};
    p4_open_envelope_ = {};
    clear_strings();
}

const LocalMediaP3HandoffStatus& LocalMediaP3HandoffBridge::status() const noexcept {
    return status_;
}

void LocalMediaP3HandoffBridge::clear_strings() noexcept {
    status_.candidate_kind[0] = '\0';
    status_.handoff_id[0] = '\0';
    status_.candidate_set_id[0] = '\0';
}

}  // namespace kivo::video::playback_graph::local_media
