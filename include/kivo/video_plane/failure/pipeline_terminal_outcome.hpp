// pipeline_terminal_outcome.hpp — GATE-008: P5A pipeline terminal outcome (6 values)
#pragma once

namespace kivo::video_plane::failure {

enum class TerminalOutcome {
    CleanShutdown,                          // pipeline shut down cleanly
    FatalShutdown,                          // pipeline shut down due to fatal signal
    FailToUpperLayer,                       // failure propagated to upper layer (P4)
    StandbyUntilNewInput,                   // pipeline entered standby mode
    Quarantined,                            // backend quarantined due to repeated failure
    ForceTerminated                         // force-terminated by watchdog
};

}  // namespace kivo::video_plane::failure
