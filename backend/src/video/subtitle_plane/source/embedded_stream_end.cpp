#include "video/subtitle_plane/source/embedded_stream_end.hpp"

namespace kivo::video::subtitle_plane::runtime {

KivoSubtitleCommandImmediateStatus apply_embedded_stream_end(
    PlaneStateRecord& plane,
    const KivoEmbeddedSubtitleStreamEndRequest& request) noexcept {
    if (request.packet_generation.packet_generation < 1) {
        plane.metrics.generation_discard_count += 1;
        return KivoSubtitleCommandImmediateStatus_NoOp;
    }
    if ((request.flags & 1u) != 0u) {
        plane.state = KivoSubtitlePlaneState_EosReached;
    }
    if ((request.flags & 2u) != 0u) {
        plane.metrics.packet_drop_count += 1;
    }
    return KivoSubtitleCommandImmediateStatus_Accepted;
}

}  // namespace kivo::video::subtitle_plane::runtime
