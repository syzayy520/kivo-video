#pragma once

#include "video/subtitle_plane/core/plane_state_record.hpp"

namespace kivo::video::subtitle_plane::runtime {

KivoSubtitleCommandToken reject_command(KivoSubtitleCorrelationId correlation_id,
                                        KivoSubtitlePlaneError error) noexcept;
KivoSubtitleCommandToken accept_command(PlaneStateRecord& plane,
                                        KivoSubtitleCorrelationId correlation_id,
                                        KivoSubtitleCommandImmediateStatus status =
                                            KivoSubtitleCommandImmediateStatus_Accepted,
                                        KivoSubtitlePlaneError error =
                                            KivoSubtitlePlaneError_None) noexcept;
KivoSubtitleCommandLifecycleSnapshot query_command_snapshot(
    const PlaneStateRecord& plane,
    KivoSubtitleCommandId command_id) noexcept;

}  // namespace kivo::video::subtitle_plane::runtime
