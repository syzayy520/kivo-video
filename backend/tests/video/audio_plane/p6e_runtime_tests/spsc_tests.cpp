// spsc_tests.cpp — P6E tests: lock-free SPSC queue
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <stdexcept>

#include "video/audio_plane/runtime/spsc/lock_free_spsc_queue.hpp"

namespace rt = kivo::video::audio_plane::backend::runtime::spsc;

static int g_failed = 0;

#define P6E_RUN(name) \
    do { \
        try { name(); } catch (...) { \
            ++g_failed; \
            std::printf("  >>> FAIL: %s\n", #name); \
        } \
    } while(0)

// E7: SPSC push/pop
static void test_e07_push_pop() {
    rt::LockFreeSpscQueue<int> q(8);
    if (!q.is_lock_free()) throw std::runtime_error("must be lock-free");
    if (!q.push(42)) throw std::runtime_error("push must succeed");
    int val = 0;
    if (!q.pop(val)) throw std::runtime_error("pop must succeed");
    if (val != 42) throw std::runtime_error("value must be 42");
    if (!q.empty()) throw std::runtime_error("must be empty after pop");
    std::printf("[E07] SPSC push/pop PASS\n");
}

// E8: SPSC full/empty
static void test_e08_full_empty() {
    rt::LockFreeSpscQueue<int> q(4);
    // capacity is 4, but ring buffer can hold 3 (one slot wasted)
    if (!q.push(1)) throw std::runtime_error("push 1 failed");
    if (!q.push(2)) throw std::runtime_error("push 2 failed");
    if (!q.push(3)) throw std::runtime_error("push 3 failed");
    if (q.push(4)) throw std::runtime_error("push 4 must fail (full)");
    int v;
    if (!q.pop(v) || v != 1) throw std::runtime_error("pop must return 1");
    if (q.size() != 2) throw std::runtime_error("size must be 2");
    std::printf("[E08] SPSC full/empty PASS\n");
}

int main() {
    std::printf("=== P6E SPSC Tests ===\n\n");
    P6E_RUN(test_e07_push_pop);
    P6E_RUN(test_e08_full_empty);
    if (g_failed == 0) { std::printf("\n=== P6E SPSC: ALL PASS ===\n"); return 0; }
    else { std::printf("\n=== P6E SPSC: %d FAILED ===\n", g_failed); return 1; }
}
