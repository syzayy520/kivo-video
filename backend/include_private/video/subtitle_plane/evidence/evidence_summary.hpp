#pragma once

#include "video/subtitle_plane/core/plane_state_record.hpp"

namespace kivo::video::subtitle_plane::runtime {

void record_evidence(PlaneStateRecord& record,
                     KivoSubtitleEvidenceReason reason,
                     KivoSubtitleEvidenceSeverity severity) noexcept;

}  // namespace kivo::video::subtitle_plane::runtime
