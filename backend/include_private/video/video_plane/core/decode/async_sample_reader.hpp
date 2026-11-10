#pragma once

#include <condition_variable>
#include <functional>
#include <mutex>
#include <thread>
#include <vector>

#include "video/video_plane/core/decode/sample_view.hpp"

namespace kivo::video::core::decode {

/// Single-packet mailbox. The reader thread exclusively calls read/seek.
/// poll never waits for I/O; its borrowed packet survives until the next poll.
/// Owners must cancel external I/O before destruction if a read may be blocked.
class AsyncSampleReader {
public:
    using Read = std::function<SampleReadStatus(SampleView&)>;
    using Seek = std::function<std::int64_t(std::int64_t)>;
    static constexpr std::size_t kMaxPacketBytes = 64 * 1024 * 1024;

    AsyncSampleReader(Read read, Seek seek);
    ~AsyncSampleReader();
    AsyncSampleReader(const AsyncSampleReader&) = delete;
    AsyncSampleReader& operator=(const AsyncSampleReader&) = delete;

    SampleReadStatus poll(SampleView& out);
    /// Only with the consumer stopped and no borrowed packet in use.
    std::int64_t seek_to(std::int64_t target_us);

private:
    void run();
    Read read_;
    Seek seek_;
    std::mutex mutex_;
    std::condition_variable wake_;
    std::thread thread_;
    std::vector<std::uint8_t> packet_;
    SampleView sample_{};
    SampleReadStatus status_ = SampleReadStatus::kTryAgain;
    bool stopping_ = false;
    bool read_requested_ = true;
    bool ready_ = false;
    bool leased_ = false;
    bool seek_requested_ = false;
    bool seek_done_ = false;
    std::int64_t seek_target_ = 0;
    std::int64_t seek_landing_ = -1;
};

}  // namespace kivo::video::core::decode
