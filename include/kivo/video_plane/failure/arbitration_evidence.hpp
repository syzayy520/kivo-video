// arbitration_evidence.hpp — GATE-008: P5A arbitration evidence
#pragma once
#include <cstdint>
#include <string>
#include "pipeline_fatal_signal.hpp"
#include "pipeline_terminal_outcome.hpp"
#include "failure_correlation_id.hpp"

namespace kivo::video_plane::failure {

struct ArbitrationEvidence {
    FailureCorrelationId correlation_id;
    PipelineFatalSignal fatal_signal{PipelineFatalSignal::UnknownFatal};
    TerminalOutcome terminal_outcome{TerminalOutcome::CleanShutdown};
    std::string arbiter_decision;           // human-readable arbiter decision
    std::string evidence_id;
    uint64_t timestamp{0};
    bool operator==(ArbitrationEvidence const&) const = default;
};

}  // namespace kivo::video_plane::failure
