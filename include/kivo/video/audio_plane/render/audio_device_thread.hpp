// audio_device_thread.hpp — P6A §17/§18: device thread contract (COM + waits)
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
namespace kivo::video::audio_plane::render {
struct AudioDeviceThreadContract {
    ContractMetadata metadata;
    bool is_spsc_consumer_only{true};
    bool owns_remainder_slot{true};
    bool com_multithreaded_default{true};
    bool com_init_uninit_pair_on_same_thread{true};
    bool no_com_in_static_initializer{true};
    bool no_com_in_global_constructor{true};
    bool no_com_in_backend_registry{true};
    bool no_com_in_public_contract{true};
    bool allowed_wait_wasapi_event{true};
    bool allowed_wait_shutdown_event{true};
    bool allowed_wait_bounded_device_recovery_wake{true};
    bool forbidden_wait_decode{true};
    bool forbidden_wait_control{true};
    bool forbidden_wait_evidence{true};
    bool forbidden_wait_file{true};
    bool forbidden_wait_network{true};
    bool forbidden_wait_ui{true};
    bool forbidden_wait_unbounded_mutex{true};
    bool forbidden_wait_condition_variable{true};
    bool forbidden_wait_disk_logging{true};
    bool never_waits_for_decode_thread{true};
    bool never_waits_for_control_thread{true};
    bool never_blocks_for_rebuffer{true};
    bool pcm_may_fill_bounded_silence{true};
    bool bit_perfect_silence_breaks_claim{true};
    bool passthrough_no_arbitrary_pcm_silence{true};
};
}  // namespace kivo::video::audio_plane::render
