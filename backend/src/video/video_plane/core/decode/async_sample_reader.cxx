#include "video/video_plane/core/decode/async_sample_reader.hpp"

#include <utility>

namespace kivo::video::core::decode {

AsyncSampleReader::AsyncSampleReader(Read read, Seek seek)
    : read_(std::move(read)), seek_(std::move(seek)) {
    // Start only after every mailbox member has been initialized.
    thread_ = std::thread([this] { run(); });
}

AsyncSampleReader::~AsyncSampleReader() {
    {
        std::lock_guard lock(mutex_);
        stopping_ = true;
    }
    wake_.notify_all();
    thread_.join();
}

SampleReadStatus AsyncSampleReader::poll(SampleView& out) {
    std::lock_guard lock(mutex_);
    out = {};
    if (leased_) {
        leased_ = false;
        read_requested_ = true;
        wake_.notify_all();
    }
    if (!ready_) return SampleReadStatus::kTryAgain;
    if (status_ == SampleReadStatus::kTryAgain) {
        ready_ = false;
        read_requested_ = true;
        wake_.notify_all();
    }
    if (status_ == SampleReadStatus::kOk) {
        out = sample_;
        ready_ = false;
        leased_ = true;
    }
    return status_;
}

std::int64_t AsyncSampleReader::seek_to(std::int64_t target_us) {
    std::unique_lock lock(mutex_);
    leased_ = ready_ = read_requested_ = false;
    seek_target_ = target_us;
    seek_done_ = false;
    seek_requested_ = true;
    wake_.notify_all();
    wake_.wait(lock, [this] { return seek_done_; });
    return seek_landing_;
}

void AsyncSampleReader::run() {
    std::unique_lock lock(mutex_);
    for (;;) {
        wake_.wait(lock, [this] { return stopping_ || seek_requested_ || read_requested_; });
        if (stopping_) return;
        if (seek_requested_) {
            const auto target = seek_target_;
            seek_requested_ = false;
            lock.unlock();
            std::int64_t landing = -1;
            try { landing = seek_(target); } catch (...) { /* published as failed seek */ }
            lock.lock();
            seek_landing_ = landing;
            seek_done_ = true;
            ready_ = false;
            read_requested_ = landing >= 0;
            wake_.notify_all();
            continue;
        }
        read_requested_ = false;
        lock.unlock();
        SampleView sample;
        SampleReadStatus status = SampleReadStatus::kFailed;
        try {
            status = read_(sample);
            if (status == SampleReadStatus::kOk) {
                if (!sample.data || sample.size == 0 || sample.size > kMaxPacketBytes) {
                    status = SampleReadStatus::kFailed;
                } else {
                    packet_.assign(sample.data, sample.data + sample.size);
                    sample.data = packet_.data();
                }
            }
        } catch (...) { status = SampleReadStatus::kFailed; }
        lock.lock();
        if (!seek_requested_) {
            sample_ = sample;
            status_ = status;
            ready_ = true;
        }
    }
}

}  // namespace kivo::video::core::decode
