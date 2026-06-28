// audio_evidence_topology.hpp — P6A evidence: evidence topology policy (§11.5)
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
namespace kivo::video::audio_plane::evidence {
struct AudioEvidenceTopology {
    ContractMetadata metadata;
    bool device_thread_spsc_hot{true};
    bool prepare_thread_spsc_or_mpsc{true};
    bool decode_thread_spsc_or_mpsc{true};
    bool control_thread_mutex_allowed{true};
    bool recovery_thread_bounded_queue{true};
    bool device_notification_forwards_compact_event{true};
    bool single_global_spsc_forbidden{true};
    bool operator==(const AudioEvidenceTopology&) const = default;
};
}  // namespace kivo::video::audio_plane::evidence
