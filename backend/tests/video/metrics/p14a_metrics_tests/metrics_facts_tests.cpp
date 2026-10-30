// metrics_facts_tests.cpp — P14-A tests: CPU 占比 / 分配计数 / 缓冲归还
//
// 这一组守的是"仪器的默认输出不能是错的"：
//   ① CPU 快照无效时，出版"未测到 + 理由"，而不是 0；
//   ② 墙钟不前进时，比值无意义但 CPU 时间仍如实出版；
//   ③ 每个输出缓冲都还回去了，才算 `leaked = 0` ——
//      第一版把 EOS 也减了一次，于是每轮正确的会话都报"泄漏 −1"；
//   ④ 分配计数只在 arm 之后、steady 之后才进各自的账。
//
// 钩子（全局 operator new 替换）不在本测试里：它是进程级的，
// 链进来会污染"这个测试自己分配了多少"。它的正确性由真机证据的
// `metrics.alloc.hook_installed` 与 `calls_steady` 共同证明。
#include <cstdint>
#include <cstdio>
#include <cmath>
#include <cstdlib>
#include <string>
#include <thread>
#include <vector>

#include "video/video_plane/core/metrics/alloc_probe.hpp"
#include "video/video_plane/core/metrics/buffer_lease_facts.hpp"
#include "video/video_plane/core/metrics/cpu_time_facts.hpp"

namespace metrics = kivo::video::core::metrics;

namespace {

int g_checked = 0;
int g_failed = 0;

#define CHECK(expr)                                                        \
    do {                                                                   \
        ++g_checked;                                                       \
        if (!(expr)) {                                                     \
            ++g_failed;                                                    \
            std::printf("  FAIL %s:%d  %s\n", __FILE__, __LINE__, #expr);  \
        }                                                                  \
    } while (false)

#define CHECK_NEAR(actual, expected, tolerance)                                        \
    do {                                                                               \
        ++g_checked;                                                                   \
        const double check_actual = static_cast<double>(actual);                       \
        const double check_expected = static_cast<double>(expected);                   \
        const double check_tolerance = static_cast<double>(tolerance);                 \
        if (std::fabs(check_actual - check_expected) > check_tolerance) {              \
            ++g_failed;                                                                \
            std::printf("  FAIL %s:%d  %.6f vs %.6f\n", __FILE__, __LINE__,            \
                        check_actual, check_expected);                                 \
        }                                                                              \
    } while (false)

constexpr std::int64_t kMs = 1'000'000;

// ── ① CPU 占比 ────────────────────────────────────────────────────

void t01_ratios_come_from_the_two_deltas() {
    metrics::CpuTimeSnapshot begin;
    begin.valid = true;
    begin.mono_ns = 1'000 * kMs;
    begin.thread_ns = 10 * kMs;
    begin.process_ns = 20 * kMs;

    metrics::CpuTimeSnapshot end;
    end.valid = true;
    end.mono_ns = 2'000 * kMs;   // 墙钟 1000 ms
    end.thread_ns = 110 * kMs;   // 线程 100 ms
    end.process_ns = 220 * kMs;  // 进程 200 ms（两核并行）

    const metrics::CpuTimeFacts facts = metrics::make_cpu_time_facts(begin, end);
    CHECK(facts.valid);
    CHECK(facts.reason[0] == '\0');
    CHECK(facts.wall_ns == 1'000 * kMs);
    CHECK_NEAR(facts.thread_ratio, 0.10, 1e-9);
    CHECK_NEAR(facts.process_ratio, 0.20, 1e-9);
}

void t02_invalid_snapshot_is_reported_not_zeroed() {
    metrics::CpuTimeSnapshot valid;
    valid.valid = true;
    valid.mono_ns = 1'000 * kMs;

    metrics::CpuTimeSnapshot invalid;  // 全 0、valid=false

    const metrics::CpuTimeFacts facts = metrics::make_cpu_time_facts(invalid, valid);
    CHECK(!facts.valid);
    CHECK(facts.reason[0] != '\0');  // 必须有理由，否则"没测到"会被读成"零占用"
    CHECK(facts.wall_ns == 0);
}

void t03_frozen_wall_clock_keeps_cpu_but_refuses_ratio() {
    metrics::CpuTimeSnapshot begin;
    begin.valid = true;
    begin.mono_ns = 5'000 * kMs;
    begin.thread_ns = 100 * kMs;
    begin.process_ns = 200 * kMs;

    metrics::CpuTimeSnapshot end = begin;  // 墙钟不动
    end.thread_ns = 300 * kMs;             // 但 CPU 在走

    const metrics::CpuTimeFacts facts = metrics::make_cpu_time_facts(begin, end);
    CHECK(!facts.valid);
    CHECK(facts.reason[0] != '\0');
    // CPU 时间仍然出版：读者要能看出"是分母坏了"，而不是"没花 CPU"
    CHECK(facts.thread_cpu_ns == 200 * kMs);
    CHECK_NEAR(facts.thread_ratio, 0.0, 1e-12);
}

// ── ② 缓冲归还 ────────────────────────────────────────────────────

void t04_all_buffers_returned_is_zero_leak() {
    const metrics::BufferLeaseFacts facts = metrics::make_buffer_lease_facts(481, 481);
    CHECK(facts.in_flight == 0);
    CHECK(facts.leaked == 0);
    CHECK(facts.reason[0] == '\0');
    CHECK(facts.pool_owner[0] != '\0');  // 归属必须写在证据里，不能靠读者猜
}

void t05_a_held_slot_is_a_leak() {
    const metrics::BufferLeaseFacts facts = metrics::make_buffer_lease_facts(481, 480);
    CHECK(facts.in_flight == 1);
    CHECK(facts.leaked == 1);
    CHECK(facts.reason[0] != '\0');
}

void t06_zero_frames_is_not_a_leak() {
    // 打开失败 / 零帧会话：不该被报成泄漏
    const metrics::BufferLeaseFacts facts = metrics::make_buffer_lease_facts(0, 0);
    CHECK(facts.in_flight == 0);
    CHECK(facts.leaked == 0);
    CHECK(facts.reason[0] == '\0');
}

void t07_released_more_than_received_is_flagged() {
    const metrics::BufferLeaseFacts facts = metrics::make_buffer_lease_facts(10, 12);
    CHECK(facts.in_flight == -2);
    CHECK(facts.reason[0] != '\0');  // 两个计数器打架，必须报出来
}

// ── ③ 分配计数 ────────────────────────────────────────────────────

void t08_counts_only_between_arm_and_disarm() {
    metrics::alloc_probe::reset();
    metrics::alloc_probe::note_call(64);  // 未 arm：不记
    CHECK(metrics::alloc_probe::calls_total() == 0);

    metrics::alloc_probe::arm();
    metrics::alloc_probe::note_call(64);
    metrics::alloc_probe::note_call(128);
    CHECK(metrics::alloc_probe::calls_total() == 2);
    CHECK(metrics::alloc_probe::bytes_total() == 192);
    // 还没 mark_steady：稳态账必须还是 0
    CHECK(metrics::alloc_probe::calls_steady() == 0);

    metrics::alloc_probe::mark_steady();
    metrics::alloc_probe::note_call(256);
    CHECK(metrics::alloc_probe::calls_steady() == 1);
    CHECK(metrics::alloc_probe::bytes_steady() == 256);
    CHECK(metrics::alloc_probe::calls_total() == 3);

    metrics::alloc_probe::disarm();
    metrics::alloc_probe::note_call(512);
    CHECK(metrics::alloc_probe::calls_total() == 3);  // disarm 之后不再记

    const metrics::AllocFacts facts = metrics::make_alloc_facts();
    CHECK(!facts.armed);
    // 本测试没链钩子：这个字段必须是 false。它 false 时 calls_steady 不可读 ——
    // 证据里少了这一格，"0 次分配"和"没装钩子"就分不开。
    CHECK(!facts.hook_installed);
    metrics::alloc_probe::reset();
}

// ── ④ 线程归因（P14-B）──────────────────────────────────────────

void t09_calls_are_attributed_to_the_calling_thread() {
    metrics::alloc_probe::reset();
    metrics::alloc_probe::arm();

    // 三个线程，各记 10 次。主线程不记，好让断言干净。
    std::vector<std::thread> workers;
    for (int index = 0; index < 3; ++index) {
        workers.emplace_back([] {
            for (int call = 0; call < 10; ++call) {
                metrics::alloc_probe::note_call(16);
            }
        });
    }
    for (std::thread& worker : workers) {
        worker.join();
    }

    const metrics::AllocFacts facts = metrics::make_alloc_facts();
    CHECK(facts.calls_total == 30);
    CHECK(facts.thread_count == 3);      // 三个线程各占一槽
    CHECK(facts.overflow_calls == 0);    // 8 个槽够用
    std::int64_t sum = 0;
    for (std::int32_t index = 0; index < facts.thread_count; ++index) {
        sum += facts.threads[static_cast<std::size_t>(index)].calls;
    }
    // 分桶不漏账：合计必须等于总量，否则有一批分配没归到任何线程上。
    CHECK(sum == facts.calls_total);
    metrics::alloc_probe::reset();
}

void t10_slot_table_overflow_is_counted_not_lost() {
    metrics::alloc_probe::reset();
    metrics::alloc_probe::arm();

    // 槽表 8 个：开 12 个线程，必然溢出。
    std::vector<std::thread> workers;
    for (int index = 0; index < 12; ++index) {
        workers.emplace_back([] { metrics::alloc_probe::note_call(1); });
    }
    for (std::thread& worker : workers) {
        worker.join();
    }

    const metrics::AllocFacts facts = metrics::make_alloc_facts();
    CHECK(facts.calls_total == 12);
    CHECK(facts.thread_count <= metrics::kMaxTrackedThreads);
    // 溢出**必须被记下来**：否则"只有 8 个线程分配过"和"后面 4 个丢了"分不开。
    CHECK(facts.overflow_calls == 12 - facts.thread_count);
    metrics::alloc_probe::reset();
}

void t11_announced_thread_appears_even_with_zero_calls() {
    metrics::alloc_probe::reset();
    metrics::alloc_probe::arm();

    // 一个只登记、不分配的线程 —— 正是播放线程的样子（G3a）。
    std::thread worker([] { metrics::alloc_probe::announce_thread("kivo-decode"); });
    worker.join();

    const metrics::AllocFacts facts = metrics::make_alloc_facts();
    // 关键：它必须在表里。只统计"分配过的线程"时，这个槽根本不存在，
    // 读者就分不清"零分配"和"没跑"。
    CHECK(facts.thread_count == 1);
    CHECK(facts.threads[0].calls == 0);
    CHECK(std::string(facts.threads[0].name) == "kivo-decode");
    CHECK(facts.calls_total == 0);
    metrics::alloc_probe::reset();
}

}  // namespace

int main() {
    struct Case {
        const char* name;
        void (*fn)();
    };
    const Case cases[] = {
        {"01 比值来自两次快照的差", t01_ratios_come_from_the_two_deltas},
        {"02 无效快照必须带理由，不填 0", t02_invalid_snapshot_is_reported_not_zeroed},
        {"03 墙钟不动仍出版 CPU 时间", t03_frozen_wall_clock_keeps_cpu_but_refuses_ratio},
        {"04 全部归还即零泄漏", t04_all_buffers_returned_is_zero_leak},
        {"05 少还一个就是泄漏", t05_a_held_slot_is_a_leak},
        {"06 零帧会话不算泄漏", t06_zero_frames_is_not_a_leak},
        {"07 归还多于取走要报红", t07_released_more_than_received_is_flagged},
        {"08 只在 arm 与 steady 之后计数", t08_counts_only_between_arm_and_disarm},
        {"09 分配归到调用它的线程", t09_calls_are_attributed_to_the_calling_thread},
        {"10 槽表溢出要计数不能丢", t10_slot_table_overflow_is_counted_not_lost},
        {"11 零分配的线程也要出现在证据里", t11_announced_thread_appears_even_with_zero_calls},
    };

    for (const Case& item : cases) {
        const int before = g_failed;
        item.fn();
        std::printf("%s %s\n", (g_failed == before) ? "PASS" : "FAIL", item.name);
    }
    std::printf("\n%d 项断言 / %d 项失败\n", g_checked, g_failed);
    return (g_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
