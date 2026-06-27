#pragma once

#include <atomic>
#include <mutex>
#include <vector>
#include "kivo/playback/control_plane/reader/reader_read_result.hpp"
#include "kivo/playback/control_plane/reader/reader_capability.hpp"
#include "kivo/playback/control_plane/revision/revision_stamp.hpp"
#include "kivo/playback/control_plane/revision/cancellation.hpp"
#include "kivo/playback/control_plane/primitives/value_types.hpp"
#include "kivo/playback/control_plane/primitives/source_range.hpp"

namespace kivo::playback::control_plane::runtime_fake {

/// FakeReader produces deterministic data for testing.
/// No real I/O. Thread-safe for read/seek/close.
class FakeReader
{
public:
    FakeReader(primitives::Bytes total_size = primitives::Bytes{1024 * 1024})
        : total_size_(total_size)
        , data_(static_cast<size_t>(total_size.value))
    {
        for (size_t i = 0; i < data_.size(); ++i)
            data_[i] = static_cast<std::byte>(i & 0xFF);
    }

    reader::ReaderCapability capability() const noexcept
    {
        reader::ReaderCapability cap;
        cap.seekable = true;
        cap.supports_fast_seek = true;
        cap.supports_graceful_cancel = true;
        cap.supports_destructive_cancel = true;
        cap.cancel_interrupts_blocking_io = true;
        cap.safe_to_reuse_after_cancel = true;
        cap.safe_to_reuse_after_seek = true;
        cap.supports_range_read = true;
        cap.transport_kind = reader::ReaderTransportKind::LocalSeekableFile;
        return cap;
    }

    reader::ReaderReadResult read(const revision::RevisionStamp& stamp, primitives::Bytes max_bytes)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        reader::ReaderReadResult result;
        result.stamp = stamp;

        if (cancelled_.load())
        {
            result.payload_kind = reader::ReaderReadPayloadKind::Cancelled;
            return result;
        }

        primitives::U64 remaining = total_size_.value - position_;
        if (remaining == 0)
        {
            result.payload_kind = reader::ReaderReadPayloadKind::EndMarkerOnly;
            reader::StreamEndMarker marker;
            marker.kind = buffer::StreamEndKind::EndOfFile;
            marker.stamp = stamp;
            marker.final_offset = primitives::SourceOffset{total_size_};
            result.end_marker = marker;
            return result;
        }

        primitives::U64 to_read = max_bytes.value;
        if (to_read > remaining) to_read = remaining;

        reader::ReaderDataPayload data;
        data.start_offset = primitives::SourceOffset{primitives::Bytes{position_}};
        data.bytes_read = primitives::Bytes{to_read};
        data.memory = data_.data() + position_;
        result.data = data;

        if (to_read == remaining)
        {
            result.payload_kind = reader::ReaderReadPayloadKind::DataAndEndMarker;
            reader::StreamEndMarker marker;
            marker.kind = buffer::StreamEndKind::EndOfFile;
            marker.stamp = stamp;
            marker.final_offset = primitives::SourceOffset{total_size_};
            result.end_marker = marker;
        }
        else
        {
            result.payload_kind = reader::ReaderReadPayloadKind::DataOnly;
        }

        position_ += to_read;
        return result;
    }

    bool seek(const revision::RevisionStamp& /*stamp*/, primitives::SourceOffset offset)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (offset.value.value > total_size_.value) return false;
        position_ = offset.value.value;
        return true;
    }

    void cancel() { cancelled_.store(true); }
    void close() { closed_.store(true); }
    bool is_closed() const noexcept { return closed_.load(); }

private:
    primitives::Bytes total_size_;
    std::vector<std::byte> data_;
    primitives::U64 position_{0};
    std::atomic<bool> cancelled_{false};
    std::atomic<bool> closed_{false};
    std::mutex mutex_;
};

} // namespace kivo::playback::control_plane::runtime_fake
