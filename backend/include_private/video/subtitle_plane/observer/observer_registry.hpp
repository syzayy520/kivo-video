#pragma once

#include "video/subtitle_plane/core/plane_state_record.hpp"

namespace kivo::video::subtitle_plane::runtime {

KivoP8ApiStatus subscribe_observer(PlaneStateRecord& record,
                                   const KivoSubtitleObserverCallbacks& callbacks,
                                   KivoSubtitleSubscriptionToken* out_token) noexcept;
KivoP8ApiStatus unsubscribe_observer(PlaneStateRecord& record,
                                     KivoSubtitleSubscriptionToken token) noexcept;
bool observer_mutation_blocked(const PlaneStateRecord& record) noexcept;
void dispatch_observer_event(PlaneStateRecord& record,
                             KivoSubtitleEvidenceReason reason,
                             KivoSubtitleEvidenceSeverity severity) noexcept;

}  // namespace kivo::video::subtitle_plane::runtime
