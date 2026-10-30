#include "video/video_plane/core/metrics/alloc_probe.hpp"

#include <cstdio>
#include <cstring>

#if defined(_WIN32)
#include <windows.h>
#else
#include <unistd.h>
#endif

namespace kivo::video::core::metrics {
namespace {

std::atomic<bool> g_armed{false};
std::atomic<bool> g_steady{false};
std::atomic<bool> g_hook_installed{false};
std::atomic<bool> g_suspended{false};
std::atomic<std::int64_t> g_calls_total{0};
std::atomic<std::int64_t> g_calls_steady{0};
std::atomic<std::int64_t> g_bytes_total{0};
std::atomic<std::int64_t> g_bytes_steady{0};
std::atomic<std::int64_t> g_overflow_calls{0};

/// 一个线程槽。`tid == 0` 表示空槽。
///
/// 名字在快照时才填（此时计数已 suspend），填一次就缓存 ——
/// 不能在 `note_call` 里读 `/proc`，那是在分配路径上做 I/O。
struct Slot {
    std::atomic<std::int32_t> tid{0};
    std::atomic<std::int64_t> calls{0};
    std::atomic<std::int64_t> bytes{0};
    std::atomic<std::int64_t> calls_steady{0};
    std::atomic<std::int64_t> bytes_steady{0};
    std::atomic<bool> named{false};
    char name[24] = {};
};

std::array<Slot, kMaxTrackedThreads> g_slots{};

/// 当前线程的槽号；−1 = 还没抢到。
///
/// **必须是平凡类型。** `thread_local int` 走 TLS 直接寻址，没有动态初始化、
/// 没有 `__cxa_guard`，不触碰堆。换成 `thread_local std::optional<int>` 之类，
/// 第一次分配就会去调 `operator new` —— 而这里正是 `operator new` 里面。
thread_local std::int32_t t_slot = -1;

/// 重入护栏：计数体内若再发生分配（理论上不该有），直接跳过，绝不递归。
thread_local bool t_in_hook = false;

[[nodiscard]] std::int32_t current_tid() noexcept {
#if defined(_WIN32)
    return static_cast<std::int32_t>(::GetCurrentThreadId());
#else
    return static_cast<std::int32_t>(::gettid());
#endif
}

/// 抢一个槽，或找到自己已有的那个。返回 −1 表示表满。
[[nodiscard]] std::int32_t claim_slot(std::int32_t tid) noexcept {
    for (std::int32_t index = 0; index < kMaxTrackedThreads; ++index) {
        std::int32_t expected = 0;
        if (g_slots[static_cast<std::size_t>(index)].tid.compare_exchange_strong(
                expected, tid, std::memory_order_relaxed)) {
            return index;
        }
        if (expected == tid) {
            return index;  // 同一 tid（或复用后的新线程）共用一格
        }
    }
    return -1;
}

/// 读内核 `comm`。读不到就留空 —— **空名不代表没有线程**。
void resolve_name(Slot& slot, std::int32_t tid) noexcept {
    bool expected = false;
    if (!slot.named.compare_exchange_strong(expected, true, std::memory_order_relaxed)) {
        return;
    }
#if !defined(_WIN32)
    char path[64] = {};
    std::snprintf(path, sizeof(path), "/proc/self/task/%d/comm", tid);
    if (FILE* file = std::fopen(path, "re")) {
        char buffer[24] = {};
        if (std::fgets(buffer, sizeof(buffer), file) != nullptr) {
            for (char& ch : buffer) {
                if (ch == '\n' || ch == '\r') {
                    ch = '\0';
                    break;
                }
            }
            std::memcpy(slot.name, buffer, sizeof(slot.name) - 1);
        }
        std::fclose(file);
    }
#else
    (void)slot;
    (void)tid;
#endif
}

}  // namespace

namespace alloc_probe {

void arm() noexcept {
    g_armed.store(true, std::memory_order_relaxed);
    g_steady.store(false, std::memory_order_relaxed);
}

void disarm() noexcept { g_armed.store(false, std::memory_order_relaxed); }

void mark_steady() noexcept { g_steady.store(true, std::memory_order_relaxed); }

void suspend() noexcept { g_suspended.store(true, std::memory_order_relaxed); }

void resume() noexcept { g_suspended.store(false, std::memory_order_relaxed); }

bool armed() noexcept { return g_armed.load(std::memory_order_relaxed); }

std::int64_t calls_total() noexcept { return g_calls_total.load(std::memory_order_relaxed); }

std::int64_t calls_steady() noexcept { return g_calls_steady.load(std::memory_order_relaxed); }

std::int64_t bytes_total() noexcept { return g_bytes_total.load(std::memory_order_relaxed); }

std::int64_t bytes_steady() noexcept { return g_bytes_steady.load(std::memory_order_relaxed); }

void reset() noexcept {
    g_calls_total.store(0, std::memory_order_relaxed);
    g_calls_steady.store(0, std::memory_order_relaxed);
    g_bytes_total.store(0, std::memory_order_relaxed);
    g_bytes_steady.store(0, std::memory_order_relaxed);
    g_overflow_calls.store(0, std::memory_order_relaxed);
    for (Slot& slot : g_slots) {
        slot.tid.store(0, std::memory_order_relaxed);
        slot.calls.store(0, std::memory_order_relaxed);
        slot.bytes.store(0, std::memory_order_relaxed);
        slot.calls_steady.store(0, std::memory_order_relaxed);
        slot.bytes_steady.store(0, std::memory_order_relaxed);
        slot.named.store(false, std::memory_order_relaxed);
        slot.name[0] = '\0';
    }
    // 本线程的槽号作废，下一次 `note_call` 重新抢。
    // 别的线程的 `t_slot` 会在 `note_call` 里被"tid 不匹配"检出并重抢 ——
    // 只清本线程会留下悬空槽号，那是很难查的一类脏数据。
    t_slot = -1;
    g_steady.store(false, std::memory_order_relaxed);
    g_armed.store(false, std::memory_order_relaxed);
}

void note_hook_installed() noexcept {
    g_hook_installed.store(true, std::memory_order_relaxed);
}

bool hook_installed() noexcept { return g_hook_installed.load(std::memory_order_relaxed); }

void note_call(std::int64_t size) noexcept {
    if (!g_armed.load(std::memory_order_relaxed) ||
        g_suspended.load(std::memory_order_relaxed)) {
        return;
    }
    if (t_in_hook) {
        return;
    }
    t_in_hook = true;

    g_calls_total.fetch_add(1, std::memory_order_relaxed);
    g_bytes_total.fetch_add(size, std::memory_order_relaxed);

    // 稳态标记之前的分配只进 total。标记发生在首帧渲染那一帧，
    // 此后所有分配都算稳态。这里不加锁：多一个栅栏不值得。
    const bool steady = g_steady.load(std::memory_order_relaxed);
    if (steady) {
        g_calls_steady.fetch_add(1, std::memory_order_relaxed);
        g_bytes_steady.fetch_add(size, std::memory_order_relaxed);
    }

    // ── 线程归因（P14-B）──
    const std::int32_t tid = current_tid();
    std::int32_t slot = t_slot;
    // `reset()` 会清空槽表；别的线程的 `t_slot` 此时指向一个已经不属于自己的槽。
    // 用 tid 对一次就能发现并重抢 —— 只是一次 relaxed load，热路径可承受。
    if (slot < 0 ||
        g_slots[static_cast<std::size_t>(slot)].tid.load(std::memory_order_relaxed) != tid) {
        slot = claim_slot(tid);
        t_slot = slot;
    }
    if (slot >= 0) {
        Slot& target = g_slots[static_cast<std::size_t>(slot)];
        target.calls.fetch_add(1, std::memory_order_relaxed);
        target.bytes.fetch_add(size, std::memory_order_relaxed);
        if (steady) {
            target.calls_steady.fetch_add(1, std::memory_order_relaxed);
            target.bytes_steady.fetch_add(size, std::memory_order_relaxed);
        }
    } else {
        g_overflow_calls.fetch_add(1, std::memory_order_relaxed);
    }

    t_in_hook = false;
}

}  // namespace alloc_probe

AllocFacts make_alloc_facts() noexcept {
    AllocFacts facts;
    facts.hook_installed = alloc_probe::hook_installed();
    facts.armed = alloc_probe::armed();
    facts.calls_total = alloc_probe::calls_total();
    facts.calls_steady = alloc_probe::calls_steady();
    facts.bytes_total = alloc_probe::bytes_total();
    facts.bytes_steady = alloc_probe::bytes_steady();
    facts.overflow_calls = g_overflow_calls.load(std::memory_order_relaxed);

    for (std::int32_t index = 0; index < kMaxTrackedThreads; ++index) {
        Slot& slot = g_slots[static_cast<std::size_t>(index)];
        const std::int32_t tid = slot.tid.load(std::memory_order_relaxed);
        if (tid == 0) {
            continue;
        }
        ThreadAllocFacts& out = facts.threads[static_cast<std::size_t>(facts.thread_count)];
        out.tid = tid;
        out.calls = slot.calls.load(std::memory_order_relaxed);
        out.bytes = slot.bytes.load(std::memory_order_relaxed);
        out.calls_steady = slot.calls_steady.load(std::memory_order_relaxed);
        out.bytes_steady = slot.bytes_steady.load(std::memory_order_relaxed);
        resolve_name(slot, tid);
        std::memcpy(out.name, slot.name, sizeof(out.name));
        ++facts.thread_count;
    }
    return facts;
}

}  // namespace kivo::video::core::metrics
