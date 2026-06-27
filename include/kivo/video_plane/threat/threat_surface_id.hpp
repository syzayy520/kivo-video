#pragma once

namespace kivo::video_plane::threat {

enum class ThreatSurfaceId {
    // 29 threat surfaces (from GATE-003 spec)
    MaliciousContainer,
    MaliciousCodecBitstream,
    MaliciousCodecExtradata,
    MaliciousSubtitlePacket,
    MaliciousAttachment,
    MaliciousFontFile,
    MaliciousHdrMetadata,
    MaliciousDolbyMetadata,
    TimestampOverflow,
    DurationOverflow,
    IndexCorruption,
    AllocationBomb,
    DecompressionBomb,
    PathTraversal,
    MetadataFakeMime,
    AttachmentMimeMismatch,
    LiveDiscontinuityAbuse,
    InfiniteProbe,
    DemuxStuck,
    DecoderStuck,
    ShaderMetadataAbuse,
    ProtectedContentExportAttempt,
    ExternalSubtitlePathAbuse,
    DebugDumpLeakage,
    SandboxIpcOversizedPayload,
    SharedMemoryStaleHandle,
    PluginCodeInjection,
    TelemetryPrivacyLeak,
    DiagnosticsPrivatePathLeak
};

}  // namespace kivo::video_plane::threat
