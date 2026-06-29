#include "video/subtitle_plane/timeline/timeline_revision.hpp"

namespace kivo::video::subtitle_plane::runtime {

RevisionRelation compare_playback_revision(const PlaneStateRecord& plane,
                                           const KivoP8PlaybackRevision request) noexcept {
    if (request.value < plane.latest_playback_revision.value) {
        return RevisionRelation::Stale;
    }
    if (request.value > plane.latest_playback_revision.value) {
        return RevisionRelation::Newer;
    }
    return RevisionRelation::Equal;
}

}  // namespace kivo::video::subtitle_plane::runtime
