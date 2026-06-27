// pipeline_fatal_signal.hpp — GATE-008: P5A pipeline fatal signal (11 values)
#pragma once

namespace kivo::video_plane::failure {

enum class PipelineFatalSignal {
    DemuxFatalCorruption,                   // demux layer unrecoverable corruption
    DecodeFatalCorruption,                  // decode layer unrecoverable corruption
    GpuDeviceRemoved,                       // GPU device removed (D3D11 device-lost)
    GpuHang,                                // GPU hang detected
    SandboxCrash,                           // sandboxed process crashed
    SandboxTimeout,                         // sandboxed process timed out
    AllocationLimitExceeded,                // allocation limit exceeded
    ProtectedContentViolation,              // protected content policy violated
    BootstrapTimeout,                       // P5 bootstrap timeout
    InternalInvariantViolation,             // P5 internal invariant violated
    UnknownFatal                            // unclassified fatal signal
};

}  // namespace kivo::video_plane::failure
