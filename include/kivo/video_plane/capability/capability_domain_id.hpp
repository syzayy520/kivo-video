#pragma once

namespace kivo::video_plane::capability {

enum class CapabilityDomainId {
    // 25 capability domains (list-authoritative, NOT text "21")
    Demux = 0,                  //  0
    FfmpegBackend,              //  1
    SandboxBroker,              //  2
    Codec,                      //  3
    HardwareDecode,             //  4
    SoftwareDecode,             //  5
    D3d11va,                    //  6
    Nv12Srv,                    //  7
    P010IntegerPlaneView,       //  8
    P010UnormFallback,          //  9
    ZeroCopy,                   // 10
    CrossAdapterBridge,         // 11
    SwapChain,                  // 12
    WaitableSwapChain,          // 13
    Vrr,                        // 14
    DwmTiming,                  // 15
    Hdr10,                      // 16
    Hdr10Plus,                  // 17
    Hlg,                        // 18
    DolbyVisionFallback,        // 19
    ProtectedPlayback,          // 20
    OutputProtection,           // 21
    SnapshotExport,             // 22
    SubtitleAttachment,         // 23
    FramePacing                 // 24
};

}  // namespace kivo::video_plane::capability
