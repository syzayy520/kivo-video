// audio_spsc_queue_contract.hpp — P6A §15: SPSC render queue interface
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
#include "kivo/video/audio_plane/render/audio_render_packet.hpp"
#include "kivo/video/audio_plane/render/audio_spsc_result.hpp"
namespace kivo::video::audio_plane::render {
class AudioSpscRenderQueueContract {
public:
    virtual ~AudioSpscRenderQueueContract() = default;
    virtual AudioSpscPushResult try_push(AudioRenderPacket&& packet) noexcept = 0;
    virtual AudioSpscPopResult try_pop(AudioRenderPacket& out) noexcept = 0;
    virtual AudioSpscDepthSnapshot depth_snapshot() const noexcept = 0;
    virtual bool is_lock_free() const noexcept = 0;
};
struct AudioSpscQueueContract {
    ContractMetadata metadata;
    bool producer_prepare_thread_only{true};
    bool consumer_device_thread_only{true};
    bool production_lock_free_required{true};
    bool no_spurious_failure_in_default_impl{true};
    bool try_again_not_needed_in_default{true};
};
}  // namespace kivo::video::audio_plane::render
