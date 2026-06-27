#pragma once

namespace kivo::playback::control_plane::recovery {

/// PlaybackFailureKind enumerates playback failure kinds.
/// Sole authority: recovery/playback_failure_kind.hpp
enum class PlaybackFailureKind
{
    ReaderOpenFailed,
    ReaderReadFailed,
    ReaderSeekFailed,
    ReaderTimeout,
    ReaderStalled,
    LowSpeedViolation,
    BufferPushRejected,
    PipelineError,
    DownstreamError,
    MemoryPoolExhausted,
    ResourceBudgetExceeded,
    NetworkError,
    TlsError,
    DrmLicenseError,
    DrmDecryptError,
    AuthenticationError,
    ManifestError,
    StreamRevisionConflict,
    Unknown
};

} // namespace kivo::playback::control_plane::recovery
