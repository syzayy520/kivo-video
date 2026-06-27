#pragma once

namespace kivo::video_plane::sandbox {

enum class DemuxBufferPlacement {
    HostProcessPool,     // buffer in host process pool
    SandboxProcessPool,  // buffer in sandbox process pool
    SharedMemoryRing,    // shared memory ring buffer
    CopyBridge           // copy across boundary
};

}  // namespace kivo::video_plane::sandbox
