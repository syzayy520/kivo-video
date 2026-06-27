#pragma once

namespace kivo::video_plane::sandbox {

enum class SandboxBrokerMessageKind {
    // 13 broker messages (from GATE-004)
    OpenStream,               // open media stream
    CloseStream,              // close media stream
    ProbeContainer,           // probe container format
    FindStreamInfo,           // find stream info
    ReadPacket,               // read next packet
    Seek,                     // seek to position
    Flush,                    // flush buffers
    Cancel,                   // cancel current operation
    GetTrackDescriptors,      // get track metadata
    GetAttachmentDescriptor,  // get attachment metadata
    GetOpaqueAttachmentChunk, // read attachment chunk
    ReportBackendEvidence,    // report backend evidence to broker
    Terminate                 // terminate sandbox process
};

}  // namespace kivo::video_plane::sandbox
