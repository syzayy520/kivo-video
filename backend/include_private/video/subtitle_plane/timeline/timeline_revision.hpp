#pragma once

#include "video/subtitle_plane/core/plane_state_record.hpp"

namespace kivo::video::subtitle_plane::runtime {

enum class RevisionRelation {
    Equal,
    Stale,
    Newer
};

RevisionRelation compare_playback_revision(const PlaneStateRecord& plane,
                                           KivoP8PlaybackRevision request) noexcept;

}  // namespace kivo::video::subtitle_plane::runtime
