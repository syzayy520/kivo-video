#include "video/video_plane/core/decode/async_sample_reader.hpp"

#include <atomic>
#include <chrono>
#include <cstdio>
#include <future>
#include <memory>
#include <stdexcept>

using namespace std::chrono_literals;
using kivo::video::core::decode::AsyncSampleReader;
using kivo::video::core::decode::SampleReadStatus;
using kivo::video::core::decode::SampleView;

namespace {
int assertions = 0;
void require(bool ok, const char* reason) {
    ++assertions;
    if (!ok) throw std::runtime_error(reason);
}

SampleReadStatus await_sample(AsyncSampleReader& reader, SampleView& sample) {
    const auto deadline = std::chrono::steady_clock::now() + 2s;
    for (;;) {
        const auto result = reader.poll(sample);
        if (result != SampleReadStatus::kTryAgain) return result;
        if (std::chrono::steady_clock::now() >= deadline)
            throw std::runtime_error("reader did not publish within 2 seconds");
        std::this_thread::yield();
    }
}

void blocked_read_and_lease() {
    std::promise<void> entered, release;
    auto released = release.get_future().share();
    std::atomic<int> calls{0};
    std::uint8_t bytes[]{11, 22, 33};
    AsyncSampleReader reader([&](SampleView& out) {
        if (++calls == 1) {
            entered.set_value();
            released.wait();
            out = {bytes, sizeof(bytes), 1234, true, true};
            return SampleReadStatus::kOk;
        }
        return SampleReadStatus::kEndOfStream;
    }, [](auto t) { return t; });
    entered.get_future().wait();
    SampleView sample;
    const auto began = std::chrono::steady_clock::now();
    bool all_pending = true;
    for (int i = 0; i < 1000; ++i)
        all_pending &= reader.poll(sample) == SampleReadStatus::kTryAgain;
    const auto poll_time = std::chrono::steady_clock::now() - began;
    release.set_value(); // Always release I/O before any assertion can unwind.
    require(all_pending && poll_time < 100ms, "poll blocked on network I/O");
    require(await_sample(reader, sample) == SampleReadStatus::kOk, "packet missing");
    require(sample.size == 3 && sample.presentation_time_us == 1234 &&
            sample.is_sync && sample.is_encrypted, "packet metadata changed");
    require(sample.data != bytes, "packet still borrows extractor storage");
    bytes[0] = 99;
    require(sample.data[0] == 11 && sample.data[2] == 33, "leased packet mutated");
    require(calls == 1, "reader prefetched while packet was leased");
    require(await_sample(reader, sample) == SampleReadStatus::kEndOfStream, "EOS missing");
    for (int i = 0; i < 100; ++i)
        require(reader.poll(sample) == SampleReadStatus::kEndOfStream, "EOS not sticky");
    require(calls == 2, "EOS triggered repeated reads");
}

void seek_serializes_with_read() {
    std::promise<void> entered, release;
    auto released = release.get_future().share();
    std::atomic<int> calls{0}, in_io{0}, overlap{0};
    std::thread::id owner;
    std::int64_t position = 1;
    std::uint8_t bytes[]{7};
    AsyncSampleReader reader([&](SampleView& out) {
        if (++in_io != 1) ++overlap;
        if (++calls == 1) {
            owner = std::this_thread::get_id();
            entered.set_value();
            released.wait();
        } else if (owner != std::this_thread::get_id()) ++overlap;
        out = {bytes, 1, position};
        --in_io;
        return SampleReadStatus::kOk;
    }, [&](auto target) {
        if (++in_io != 1 || owner != std::this_thread::get_id()) ++overlap;
        position = target - 10;
        --in_io;
        return position;
    });
    entered.get_future().wait();
    auto seeking = std::async(std::launch::async, [&] { return reader.seek_to(1000); });
    const auto before_release = seeking.wait_for(30ms);
    release.set_value();
    require(before_release == std::future_status::timeout, "seek raced blocked read");
    require(seeking.get() == 990, "seek lost actual landing");
    SampleView sample;
    require(await_sample(reader, sample) == SampleReadStatus::kOk &&
            sample.presentation_time_us == 990, "seek leaked old ready packet");
    require(overlap == 0, "extractor read/seek ownership overlapped");
}

void ready_packet_discard_and_seek_failure() {
    std::uint8_t byte = 3;
    std::int64_t position = 1;
    AsyncSampleReader reader([&](SampleView& out) {
        out = {&byte, 1, position};
        return SampleReadStatus::kOk;
    }, [&](auto target) -> std::int64_t {
        if (target == 999) throw std::runtime_error("seek failure");
        position = target;
        return position;
    });
    SampleView sample;
    require(await_sample(reader, sample) == SampleReadStatus::kOk, "initial packet missing");
    require(reader.seek_to(500) == 500, "seek from lease failed");
    require(await_sample(reader, sample) == SampleReadStatus::kOk &&
            sample.presentation_time_us == 500, "lease survived seek");
    require(reader.seek_to(999) == -1, "throwing seek reported success");
    require(reader.poll(sample) == SampleReadStatus::kTryAgain, "failed seek leaked packet");
    require(reader.seek_to(700) == 700, "reader could not recover after failed seek");
    require(await_sample(reader, sample) == SampleReadStatus::kOk &&
            sample.presentation_time_us == 700, "recovery seek packet missing");
}

void failed_reads() {
    for (int kind = 0; kind < 5; ++kind) {
        std::atomic<int> calls{0};
        std::uint8_t byte = 1;
        AsyncSampleReader reader([&](SampleView& out) {
            ++calls;
            if (kind == 0) return SampleReadStatus::kFailed;
            if (kind == 1) throw std::runtime_error("read failure");
            out.data = kind == 2 ? nullptr : &byte;
            out.size = kind == 3 ? 0 : kind == 4 ? AsyncSampleReader::kMaxPacketBytes + 1 : 1;
            return SampleReadStatus::kOk;
        }, [](auto) { return -1; });
        SampleView sample;
        require(await_sample(reader, sample) == SampleReadStatus::kFailed, "invalid read accepted");
        require(reader.poll(sample) == SampleReadStatus::kFailed && !sample.data,
                "failed read published borrowed bytes");
        require(calls == 1, "failed read spun forever");
    }
}

void retry_and_join() {
    std::atomic<int> calls{0};
    {
        AsyncSampleReader reader([&](SampleView&) {
            return ++calls == 1 ? SampleReadStatus::kTryAgain : SampleReadStatus::kEndOfStream;
        }, [](auto t) { return t; });
        SampleView sample;
        require(await_sample(reader, sample) == SampleReadStatus::kEndOfStream,
                "transient read did not retry");
        require(calls == 2, "unexpected retry count");
    }
    std::promise<void> entered, release;
    auto released = release.get_future().share();
    std::atomic<bool> exited{false};
    auto reader = std::make_unique<AsyncSampleReader>([&](SampleView&) {
        entered.set_value();
        released.wait();
        exited = true;
        return SampleReadStatus::kFailed;
    }, [](auto t) { return t; });
    entered.get_future().wait();
    auto closing = std::async(std::launch::async, [&] { reader.reset(); });
    const auto before_cancel = closing.wait_for(30ms);
    release.set_value(); // Represents external HTTP cancellation, required before join.
    closing.get();
    require(before_cancel == std::future_status::timeout && exited,
            "destructor detached reader or failed to join after cancellation");
}
} // namespace

int main() {
    try {
        blocked_read_and_lease();
        seek_serializes_with_read();
        ready_packet_discard_and_seek_failure();
        failed_reads();
        retry_and_join();
        std::printf("PASS async sample reader: 5 groups / %d assertions\n", assertions);
        return 0;
    } catch (const std::exception& e) {
        std::fprintf(stderr, "FAIL async sample reader: %s\n", e.what());
        return 1;
    }
}
