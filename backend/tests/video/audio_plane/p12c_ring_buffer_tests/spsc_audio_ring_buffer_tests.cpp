// spsc_audio_ring_buffer_tests.cpp — P12-C2 tests: SpscAudioRingBuffer (R1-R9)
//
// 这一组测试的唯一目的：证明"一个生产线程 + 一个消费线程"之间
// 既不丢样本、也不重样本、也不乱序，并且在热路径上不做任何分配。
// 任何一条不成立，换到真机上就是爆音或时钟漂移，而且现场不可复现。
#include <atomic>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <new>
#include <stdexcept>
#include <thread>
#include <vector>

#include "video/video_plane/core/audio/spsc_audio_ring_buffer.hpp"

namespace ring = kivo::video::core::audio;

// ── 全局分配计数：用于断言热路径零分配（R7） ──
// 只有测试可执行文件这么做；被测代码本身对分配无感知。
namespace {
std::atomic<long long> g_alloc_calls{0};
}

void* operator new(std::size_t size) {
    g_alloc_calls.fetch_add(1, std::memory_order_relaxed);
    if (void* pointer = std::malloc(size == 0 ? 1 : size)) {
        return pointer;
    }
    throw std::bad_alloc();
}

void* operator new[](std::size_t size) {
    g_alloc_calls.fetch_add(1, std::memory_order_relaxed);
    if (void* pointer = std::malloc(size == 0 ? 1 : size)) {
        return pointer;
    }
    throw std::bad_alloc();
}

void operator delete(void* pointer) noexcept {
    std::free(pointer);
}

void operator delete[](void* pointer) noexcept {
    std::free(pointer);
}

void operator delete(void* pointer, std::size_t) noexcept {
    std::free(pointer);
}

void operator delete[](void* pointer, std::size_t) noexcept {
    std::free(pointer);
}

namespace {

int g_failed = 0;
int g_checked = 0;

void expect(bool condition, const char* what) {
    ++g_checked;
    if (!condition) {
        ++g_failed;
        std::printf("  [FAIL] %s\n", what);
    }
}

void expect_eq(std::int64_t actual, std::int64_t expected, const char* what) {
    ++g_checked;
    if (actual != expected) {
        ++g_failed;
        std::printf("  [FAIL] %s: 实际 %lld，期望 %lld\n", what,
                    static_cast<long long>(actual), static_cast<long long>(expected));
    }
}

/// 把序号编码成 float。小于 1e6 的整数在 float 里可精确表示，因此比较是严格的。
float encoded(std::int32_t sequence) {
    return static_cast<float>(sequence);
}

/// R1：最基本的写入 → 读出，逐样本相等。
void r1_basic_write_read() {
    ring::SpscAudioRingBuffer buffer(1, 64);
    std::vector<float> in(100);
    for (std::int32_t i = 0; i < 100; ++i) {
        in[static_cast<std::size_t>(i)] = encoded(i);
    }

    std::vector<float> out(100, 0.0f);
    // 容量 64：第一次只能写 64，必须如实返回短写
    expect_eq(buffer.write(in.data(), 100), 64, "R1 首轮写应短写为容量 64");
    expect_eq(buffer.read(out.data(), 100), 64, "R1 首轮读应读到 64 帧");
    for (std::int32_t i = 0; i < 64; ++i) {
        expect(out[static_cast<std::size_t>(i)] == encoded(i), "R1 首轮样本顺序");
    }

    // 剩余 36 帧现在有空位了
    expect_eq(buffer.write(in.data() + 64, 36), 36, "R1 次轮写应成功 36 帧");
    expect_eq(buffer.read(out.data(), 100), 36, "R1 次轮读应读到 36 帧");
    for (std::int32_t i = 0; i < 36; ++i) {
        expect(out[static_cast<std::size_t>(i)] == encoded(64 + i), "R1 次轮样本顺序");
    }
}

/// R2：跨越物理末尾的环绕读写，顺序不能断。
void r2_wrap_around() {
    ring::SpscAudioRingBuffer buffer(1, 16);
    std::int32_t sequence = 0;
    std::vector<float> out(16, -1.0f);

    // 反复"写满 → 读空"，强制索引多次跨过末尾
    for (int round = 0; round < 8; ++round) {
        std::vector<float> in(16);
        for (std::int32_t i = 0; i < 16; ++i) {
            in[static_cast<std::size_t>(i)] = encoded(sequence + i);
        }
        expect_eq(buffer.write(in.data(), 16), 16, "R2 整轮写满");
        expect_eq(buffer.read(out.data(), 16), 16, "R2 整轮读空");
        for (std::int32_t i = 0; i < 16; ++i) {
            expect(out[static_cast<std::size_t>(i)] == encoded(sequence + i), "R2 环绕后顺序");
        }
        sequence += 16;

        // 再错开半个容量，让索引不总是落在边界上
        std::vector<float> offset(7);
        for (std::int32_t i = 0; i < 7; ++i) {
            offset[static_cast<std::size_t>(i)] = encoded(sequence + i);
        }
        expect_eq(buffer.write(offset.data(), 7), 7, "R2 错位写");
        expect_eq(buffer.read(out.data(), 7), 7, "R2 错位读");
        for (std::int32_t i = 0; i < 7; ++i) {
            expect(out[static_cast<std::size_t>(i)] == encoded(sequence + i), "R2 错位后顺序");
        }
        sequence += 7;
    }
}

/// R3：空间不足时返回短写，不阻塞、不越界。
void r3_short_write_when_full() {
    ring::SpscAudioRingBuffer buffer(1, 32);
    std::vector<float> in(32, 1.0f);
    expect_eq(buffer.write(in.data(), 32), 32, "R3 写满 32");
    expect_eq(buffer.write(in.data(), 1), 0, "R3 满时写 1 帧应返回 0");
    expect_eq(buffer.free_frames(), 0, "R3 满时可写帧数为 0");

    std::vector<float> out(32, 0.0f);
    expect_eq(buffer.read(out.data(), 16), 16, "R3 读走一半");
    expect_eq(buffer.write(in.data(), 100), 16, "R3 只剩 16 帧空间，应短写为 16");
    expect_eq(buffer.available_frames(), 32, "R3 之后再读应仍为满容量");
}

/// R4：缓冲为空时读取返回 0，且不得改动输出缓冲。
void r4_empty_read_does_not_touch_output() {
    ring::SpscAudioRingBuffer buffer(2, 32);
    std::vector<float> out(16 * 2, 12345.0f);
    expect_eq(buffer.read(out.data(), 16), 0, "R4 空读应返回 0");
    for (float value : out) {
        expect(value == 12345.0f, "R4 空读不得修改输出缓冲");
    }

    // 空指针/非法参数必须是"返回 0"，而不是崩溃
    expect_eq(buffer.read(nullptr, 16), 0, "R4 空指针读返回 0");
    expect_eq(buffer.write(nullptr, 16), 0, "R4 空指针写返回 0");
    expect_eq(buffer.read(out.data(), 0), 0, "R4 零帧读返回 0");
}

/// R5：写满后继续写绝不能覆盖未读数据（这是环形缓冲最经典的 bug）。
void r5_never_overwrites_unread() {
    ring::SpscAudioRingBuffer buffer(1, 8);
    std::vector<float> first(8);
    for (std::int32_t i = 0; i < 8; ++i) {
        first[static_cast<std::size_t>(i)] = encoded(1000 + i);
    }
    expect_eq(buffer.write(first.data(), 8), 8, "R5 写满 8");

    std::vector<float> intruder(8, -999.0f);
    expect_eq(buffer.write(intruder.data(), 8), 0, "R5 满时再写 8 应全被拒绝");

    std::vector<float> out(8, 0.0f);
    expect_eq(buffer.read(out.data(), 8), 8, "R5 读回 8");
    for (std::int32_t i = 0; i < 8; ++i) {
        expect(out[static_cast<std::size_t>(i)] == encoded(1000 + i), "R5 旧数据不得被覆盖");
    }
}

/// R6：并发——一个生产者灌 100 万帧，一个消费者逐帧校验顺序，不得丢/重/乱。
void r6_concurrent_one_million() {
    constexpr std::int32_t kTotalFrames = 1'000'000;
    constexpr std::int32_t kProducerChunk = 100;
    constexpr std::int32_t kConsumerChunk = 37;  // 故意与生产者不同，制造半包读写

    ring::SpscAudioRingBuffer buffer(1, 256);
    std::atomic<std::int64_t> written{0};
    std::atomic<std::int64_t> read_back{0};
    std::int64_t mismatches = 0;
    std::int64_t first_mismatch_at = -1;

    std::thread producer([&] {
        std::vector<float> chunk(static_cast<std::size_t>(kProducerChunk));
        std::int32_t sequence = 0;
        while (sequence < kTotalFrames) {
            const std::int32_t remaining = kTotalFrames - sequence;
            const std::int32_t want = remaining < kProducerChunk ? remaining : kProducerChunk;
            for (std::int32_t i = 0; i < want; ++i) {
                chunk[static_cast<std::size_t>(i)] = encoded(sequence + i);
            }
            const std::int32_t accepted = buffer.write(chunk.data(), want);
            if (accepted == 0) {
                std::this_thread::yield();
                continue;
            }
            sequence += accepted;
            written.fetch_add(accepted, std::memory_order_relaxed);
        }
    });

    std::thread consumer([&] {
        std::vector<float> chunk(static_cast<std::size_t>(kConsumerChunk));
        std::int32_t expected_sequence = 0;
        while (expected_sequence < kTotalFrames) {
            const std::int32_t got = buffer.read(chunk.data(), kConsumerChunk);
            if (got == 0) {
                std::this_thread::yield();
                continue;
            }
            for (std::int32_t i = 0; i < got; ++i) {
                if (chunk[static_cast<std::size_t>(i)] != encoded(expected_sequence + i)) {
                    if (mismatches == 0) {
                        first_mismatch_at = expected_sequence + i;
                    }
                    ++mismatches;
                }
            }
            expected_sequence += got;
            read_back.fetch_add(got, std::memory_order_relaxed);
        }
    });

    producer.join();
    consumer.join();

    expect_eq(written.load(), kTotalFrames, "R6 生产者写入总数");
    expect_eq(read_back.load(), kTotalFrames, "R6 消费者读出总数");
    expect_eq(mismatches, 0, "R6 样本不得乱序/损坏");
    if (mismatches != 0) {
        std::printf("       首个不一致位置：%lld\n", static_cast<long long>(first_mismatch_at));
    }
}

/// R7：稳态热路径零分配（对齐 G3 的口径）。
void r7_no_allocation_in_hot_path() {
    ring::SpscAudioRingBuffer buffer(2, 512);
    std::vector<float> in(512 * 2, 0.5f);
    std::vector<float> out(512 * 2, 0.0f);

    // 预热：第一轮允许任何一次性初始化
    buffer.write(in.data(), 512);
    buffer.read(out.data(), 512);

    const long long before = g_alloc_calls.load(std::memory_order_relaxed);
    for (int i = 0; i < 10000; ++i) {
        buffer.write(in.data(), 512);
        buffer.read(out.data(), 512);
    }
    const long long after = g_alloc_calls.load(std::memory_order_relaxed);

    expect_eq(after - before, 0, "R7 热路径 1 万轮零分配");
}

/// R8：容量不是 2 的幂时向上取整，且实际容量不小于请求值。
void r8_capacity_rounds_up_to_power_of_two() {
    ring::SpscAudioRingBuffer odd(2, 100);
    expect_eq(odd.capacity_frames(), 128, "R8 请求 100 帧应取整到 128");
    expect(odd.capacity_frames() >= 100, "R8 实际容量不得小于请求");

    ring::SpscAudioRingBuffer exact(2, 64);
    expect_eq(exact.capacity_frames(), 64, "R8 已是 2 的幂应保持不变");

    ring::SpscAudioRingBuffer tiny(1, 1);
    expect_eq(tiny.capacity_frames(), 1, "R8 最小容量 1 帧");

    // 非法参数必须抛出，而不是静默造出一个坏缓冲
    bool threw_channel = false;
    try {
        ring::SpscAudioRingBuffer bad(0, 64);
    } catch (const std::invalid_argument&) {
        threw_channel = true;
    }
    expect(threw_channel, "R8 声道数为 0 应抛出 invalid_argument");

    bool threw_capacity = false;
    try {
        ring::SpscAudioRingBuffer bad(2, 0);
    } catch (const std::invalid_argument&) {
        threw_capacity = true;
    }
    expect(threw_capacity, "R8 容量为 0 应抛出 invalid_argument");
}

/// R9：立体声交错读写不得串道（左右声道错位是音频最刺耳的 bug）。
void r9_stereo_interleaving_keeps_channels() {
    constexpr std::int32_t kFrames = 24;
    constexpr std::int32_t kChannels = 2;
    ring::SpscAudioRingBuffer buffer(kChannels, 32);

    std::vector<float> in(static_cast<std::size_t>(kFrames) * kChannels);
    for (std::int32_t frame = 0; frame < kFrames; ++frame) {
        in[static_cast<std::size_t>(frame) * kChannels] = encoded(frame);       // L
        in[static_cast<std::size_t>(frame) * kChannels + 1] = encoded(-frame);  // R
    }

    expect_eq(buffer.write(in.data(), kFrames), kFrames, "R9 写入 24 帧立体声");

    std::vector<float> out(static_cast<std::size_t>(kFrames) * kChannels, 0.0f);
    expect_eq(buffer.read(out.data(), kFrames), kFrames, "R9 读出 24 帧立体声");
    for (std::int32_t frame = 0; frame < kFrames; ++frame) {
        expect(out[static_cast<std::size_t>(frame) * kChannels] == encoded(frame), "R9 左声道");
        expect(out[static_cast<std::size_t>(frame) * kChannels + 1] == encoded(-frame), "R9 右声道");
    }
}

struct TestCase {
    const char* name;
    void (*run)();
};

const TestCase kCases[] = {
    {"R1 基本写读", r1_basic_write_read},
    {"R2 环绕顺序", r2_wrap_around},
    {"R3 写满短写", r3_short_write_when_full},
    {"R4 空读不改输出", r4_empty_read_does_not_touch_output},
    {"R5 不覆盖未读", r5_never_overwrites_unread},
    {"R6 并发 100 万帧", r6_concurrent_one_million},
    {"R7 热路径零分配", r7_no_allocation_in_hot_path},
    {"R8 容量取整与非法参数", r8_capacity_rounds_up_to_power_of_two},
    {"R9 立体声不串道", r9_stereo_interleaving_keeps_channels},
};

}  // namespace

int main() {
    std::printf("=== P12-C2 SpscAudioRingBuffer tests ===\n");
    for (const auto& test_case : kCases) {
        const int failed_before = g_failed;
        test_case.run();
        std::printf("%s %s\n", g_failed == failed_before ? "[PASS]" : "[FAIL]", test_case.name);
    }
    std::printf("--- 断言 %d 项，失败 %d 项 ---\n", g_checked, g_failed);
    return g_failed == 0 ? 0 : 1;
}
