#pragma once

namespace kivo::video_plane::evidence {

enum class EvidenceSeverity {
    Trace,              // trace-level detail
    Debug,              // debug-level detail
    Info,               // informational
    Warning,            // warning condition
    RecoverableError,   // error, recovered
    Fatal,              // fatal error
    Security,           // security-relevant
    PolicyDenied,       // action denied by policy
    FailClosed          // failed, closed to prevent harm
};

}  // namespace kivo::video_plane::evidence
