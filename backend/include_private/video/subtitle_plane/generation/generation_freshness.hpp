#pragma once

#include "kivo/video/subtitle_plane/kivo_subtitle_plane.h"

namespace kivo::video::subtitle_plane::runtime {

bool delay_change_keeps_static_cues(KivoSubtitleTimelineGeneration old_generation,
                                    KivoSubtitleTimelineGeneration new_generation) noexcept;
bool static_external_index_survives_seek(KivoSubtitleContentGeneration content) noexcept;
bool embedded_packet_matches_generation(KivoSubtitlePacketGeneration expected,
                                        KivoSubtitlePacketGeneration packet) noexcept;
bool cold_start_accepts_packet(KivoSubtitleTimelineAvailability availability) noexcept;

}  // namespace kivo::video::subtitle_plane::runtime
