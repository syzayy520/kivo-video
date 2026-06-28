// audio_concurrency_stress_contract.hpp — P6A test_contract: stress (§test_contract)
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
namespace kivo::video::audio_plane::test_contract {

// AudioConcurrencyStressContract
// Contract for concurrency stress tests. Compile-only skeleton.
struct AudioConcurrencyStressContract {
    kivo::video::audio_plane::ContractMetadata metadata{};
    bool seek_while_device_lost{true};
    bool track_switch_while_format_changed{true};
    bool shutdown_while_decode_init{true};
    bool hotplug_during_getbuffer{true};
    bool p4_superseded_command_storm{true};
    bool evidence_queue_pressure{true};
    bool randomized_command_interleaving{true};
    bool deterministic_scheduler_fake{true};
    bool tsan_where_possible{true};

    bool operator==(const AudioConcurrencyStressContract&) const = default;
};

}  // namespace kivo::video::audio_plane::test_contract
