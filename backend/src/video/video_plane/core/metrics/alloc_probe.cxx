#include "video/video_plane/core/metrics/alloc_probe.hpp"

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
    g_calls_total.fetch_add(1, std::memory_order_relaxed);
    g_bytes_total.fetch_add(size, std::memory_order_relaxed);

    // 稳态标记之前的分配只进 total。这里**不加锁也不判断顺序**：
    // 标记发生在首帧渲染那一帧，此后所有分配都算稳态。
    if (g_steady.load(std::memory_order_relaxed)) {
        g_calls_steady.fetch_add(1, std::memory_order_relaxed);
        g_bytes_steady.fetch_add(size, std::memory_order_relaxed);
    }
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
    return facts;
}

}  // namespace kivo::video::core::metrics
