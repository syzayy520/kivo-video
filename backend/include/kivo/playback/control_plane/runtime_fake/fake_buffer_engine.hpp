#pragma once

#include <deque>
#include <mutex>
#include "kivo/playback/control_plane/buffer/buffer_engine.hpp"
#include "kivo/playback/control_plane/buffer/buffer_item.hpp"

namespace kivo::playback::control_plane::runtime_fake {

/// FakeBufferEngine is an in-memory BufferEngine for testing.
class FakeBufferEngine : public buffer::BufferEngine
{
public:
    buffer::BufferPushResult push(const buffer::BufferItem& item, const buffer::BufferPushPolicy&) override
    {
        std::lock_guard<std::mutex> lock(mutex_);
        buffer::BufferPushResult result;
        if (fenced_)
        {
            result.status = buffer::BufferPushStatus::RejectedFenced;
            return result;
        }
        items_.push_back(item);
        result.status = buffer::BufferPushStatus::Accepted;
        return result;
    }

    void wake_all_waiters(buffer::BufferWakeReason) override {}

    buffer::BufferSnapshot snapshot() const override
    {
        std::lock_guard<std::mutex> lock(mutex_);
        buffer::BufferSnapshot snap;
        snap.buffered_bytes = primitives::Bytes{items_.size() * 1024};
        snap.capacity_bytes = primitives::Bytes{16 * 1024 * 1024};
        snap.level = items_.size() > 100 ? buffer::BufferLevel::High : buffer::BufferLevel::Low;
        return snap;
    }

    void set_fenced(bool f) { std::lock_guard<std::mutex> lock(mutex_); fenced_ = f; }
    void clear() { std::lock_guard<std::mutex> lock(mutex_); items_.clear(); }
    size_t item_count() const { std::lock_guard<std::mutex> lock(mutex_); return items_.size(); }

private:
    mutable std::mutex mutex_;
    std::deque<buffer::BufferItem> items_;
    bool fenced_{false};
};

} // namespace kivo::playback::control_plane::runtime_fake
