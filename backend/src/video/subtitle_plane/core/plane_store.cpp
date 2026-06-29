#include "video/subtitle_plane/core/plane_store.hpp"

#include <array>
#include <atomic>

namespace kivo::video::subtitle_plane::runtime {
namespace {

constexpr uint64_t kMaxPlanes = 16;

struct PlaneSlot {
    std::atomic<uint32_t> active{0};
    PlaneStateRecord record{};
};

std::array<PlaneSlot, kMaxPlanes> g_slots{};

}  // namespace

PlaneStateRecord* create_plane_record(const KivoSubtitlePlaneRuntimeCaps& caps,
                                      KivoSubtitlePlaneHandle* const out_handle) noexcept {
    if (out_handle == nullptr) {
        return nullptr;
    }
    for (uint64_t index = 0; index < kMaxPlanes; ++index) {
        uint32_t expected = 0;
        PlaneSlot& slot = g_slots[static_cast<size_t>(index)];
        if (slot.active.compare_exchange_strong(expected, 2, std::memory_order_acq_rel)) {
            slot.record = PlaneStateRecord{};
            slot.record.handle_value = index + 1;
            slot.record.caps = caps;
            slot.record.state = KivoSubtitlePlaneState_Created;
            slot.record.snapshot_revision.value = 1;
            slot.record.latest_playback_revision.value = 1;
            slot.record.latest_graph_revision.value = 1;
            slot.active.store(1, std::memory_order_release);
            out_handle->value = index + 1;
            return &slot.record;
        }
    }
    out_handle->value = 0;
    return nullptr;
}

PlaneStateRecord* find_plane_record(const KivoSubtitlePlaneHandle handle) noexcept {
    if (handle.value == 0 || handle.value > kMaxPlanes) {
        return nullptr;
    }
    PlaneSlot& slot = g_slots[static_cast<size_t>(handle.value - 1)];
    return slot.active.load(std::memory_order_acquire) == 1 ? &slot.record : nullptr;
}

bool destroy_plane_record(const KivoSubtitlePlaneHandle handle) noexcept {
    PlaneStateRecord* const record = find_plane_record(handle);
    if (record == nullptr) {
        return false;
    }
    g_slots[static_cast<size_t>(handle.value - 1)].active.store(0, std::memory_order_release);
    return true;
}

}  // namespace kivo::video::subtitle_plane::runtime
