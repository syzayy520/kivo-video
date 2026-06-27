// pipeline_failure_arbiter.hpp — GATE-008: P5A pipeline failure arbiter contract
#pragma once
#include <string>
#include "pipeline_fatal_signal.hpp"
#include "pipeline_terminal_outcome.hpp"
#include "failure_correlation_id.hpp"
#include "arbitration_evidence.hpp"

namespace kivo::video_plane::failure {

struct PipelineFailureArbiter {
    // Contract:
    // - Receives PipelineFatalSignal from any P5 component
    // - Determines TerminalOutcome based on signal kind and pipeline state
    // - Emits ArbitrationEvidence with FailureCorrelationId
    // - Single arbiter instance per pipeline (single-owner rule)
    // - May NOT override P4 playback position authority
    static TerminalOutcome arbitrate(PipelineFatalSignal signal);
    static ArbitrationEvidence build_evidence(
        PipelineFatalSignal signal,
        TerminalOutcome outcome,
        FailureCorrelationId correlation_id);
    static bool is_quarantine_triggered(uint32_t consecutive_failures);
};

}  // namespace kivo::video_plane::failure
