// fake_data_handle_manager.hpp — P6B data: fake data handle manager
// HEADER-ONLY. In-process, single-threaded fake of the data handle manager.
#pragma once
#include <cstdint>
#include <cstring>
#include <unordered_map>
#include <vector>
#include "kivo/video/audio_plane/data/audio_data_handle_id.hpp"
#include "kivo/video/audio_plane/data/audio_sync_byte_view.hpp"
#include "kivo/video/audio_plane/data/audio_owned_block_handle.hpp"
#include "kivo/video/audio_plane/data/audio_pool_slot_handle.hpp"
#include "kivo/video/audio_plane/packet/audio_packet_id.hpp"

namespace kivo::video::audio_plane::runtime::data {

using kivo::video::audio_plane::data::AudioSyncByteView;
using kivo::video::audio_plane::data::AudioOwnedBlockHandle;
using kivo::video::audio_plane::data::AudioPoolSlotHandle;
using kivo::video::audio_plane::data::AudioDataOwnerStage;
using kivo::video::audio_plane::data::AudioDataHandleId;

// ReleaseResult: outcome of a handle release.
enum class ReleaseResult : int32_t {
    Released = 0,
    NotFound = 1,
    AlreadyReleased = 2,
};

// FakeDataHandleManager
// Single-threaded fake that hands out sync views, owned blocks, and pool slots.
// All storage is local std::vector / std::unordered_map; no real allocator.
class FakeDataHandleManager {
public:
    FakeDataHandleManager() = default;

    // Acquire a synchronous borrowed byte view for a packet.
    // Returns a view over fake bytes (deterministic fill, no real payload).
    AudioSyncByteView acquire_sync_view(packet::AudioPacketId packet_id) noexcept {
        const uint64_t id = next_handle_id_++;
        std::vector<std::byte> fake(default_view_size_, std::byte{0xAB});
        sync_storage_[id] = std::move(fake);
        auto& stored = sync_storage_[id];
        AudioSyncByteView view{};
        view.handle_id.value = id;
        view.bytes = std::span<const std::byte>{stored.data(), stored.size()};
        view.owner_stage = AudioDataOwnerStage::P6;
        view.packet_id = packet_id;
        active_handles_[id] = HandleKind::SyncView;
        return view;
    }

    // Acquire an owned block of the requested size for a packet.
    AudioOwnedBlockHandle acquire_owned_block(
        packet::AudioPacketId packet_id, std::size_t size) noexcept
    {
        const uint64_t id = next_handle_id_++;
        block_storage_[id].assign(size, std::byte{0xCD});
        AudioOwnedBlockHandle handle{};
        handle.handle_id.value = id;
        handle.packet_id = packet_id;
        handle.flags.is_noexcept_release = true;
        handle.flags.is_heavy_release = false;
        active_handles_[id] = HandleKind::OwnedBlock;
        return handle;
    }

    // Acquire a pool slot for a packet.
    AudioPoolSlotHandle acquire_pool_slot(packet::AudioPacketId packet_id) noexcept {
        const uint64_t id = next_handle_id_++;
        AudioPoolSlotHandle handle{};
        handle.pool_id.value = 1;  // single fake pool
        handle.slot_id.value = id;
        handle.packet_id = packet_id;
        active_handles_[id] = HandleKind::PoolSlot;
        return handle;
    }

    // Release a handle by id. Idempotent per AlreadyReleased.
    ReleaseResult release(AudioDataHandleId handle_id) noexcept {
        auto it = active_handles_.find(handle_id.value);
        if (it == active_handles_.end()) {
            // Check if it was already released.
            if (released_handles_.count(handle_id.value) > 0) {
                return ReleaseResult::AlreadyReleased;
            }
            return ReleaseResult::NotFound;
        }
        switch (it->second) {
            case HandleKind::SyncView:
                sync_storage_.erase(handle_id.value);
                break;
            case HandleKind::OwnedBlock:
                block_storage_.erase(handle_id.value);
                break;
            case HandleKind::PoolSlot:
                // Pool slots have no separate storage here.
                break;
        }
        released_handles_[handle_id.value] = 1;
        active_handles_.erase(it);
        return ReleaseResult::Released;
    }

    // Diagnostic: number of currently active handles.
    std::size_t active_count() const noexcept { return active_handles_.size(); }

private:
    enum class HandleKind : int32_t { SyncView = 0, OwnedBlock = 1, PoolSlot = 2 };

    uint64_t next_handle_id_{1};
    std::size_t default_view_size_{64};
    std::unordered_map<uint64_t, std::vector<std::byte>> sync_storage_;
    std::unordered_map<uint64_t, std::vector<std::byte>> block_storage_;
    std::unordered_map<uint64_t, HandleKind> active_handles_;
    std::unordered_map<uint64_t, uint8_t> released_handles_;
};

}  // namespace kivo::video::audio_plane::runtime::data
