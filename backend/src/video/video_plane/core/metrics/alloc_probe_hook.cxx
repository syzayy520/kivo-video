// 全局 `operator new/delete` 的替换（P14-A / G3）。
//
// **本文件只该被 Android 的原生库链接。** 桌面单测链它意味着每个测试都带上这把尺子，
// 而 G3 要测的是"播放稳态"，不是"测试框架的分配"。CMake 里它单独一个静态目标
// （`kivo_metrics_alloc_hook`），由 android 目标显式链入。
//
// 为什么需要替换而不是用 `mallinfo`：
//   `mallinfo` 只回答"堆涨了多少"。一个每帧 new 一个 vector 又马上 delete 的热路径，
//   在它眼里堆总量纹丝不动 —— 干净得像没分配。判据要的是**调用次数**。
//
// 8 个变体 + 6 个对齐变体必须**全部**替换：
//   少替换一个，那一类分配就绕过计数（少报），而且不同 TU 会看到不同的分配器 ——
//   混用 `malloc` / `posix_memalign` 的释放路径迟早出错。这也是 C++17 之后
//   这一族函数变成 14 个的原因。
//
// 兼容性：libc++ 自己的 `operator new` 内部也是调 `malloc`，所以即便符号插入
// 让别的 .so 走到这里（或反过来），两边最终都落在同一套 `malloc/free` 上，
// 不会出现"allocator 不匹配"的崩溃。代价是计数变成**进程级**的 ——
// 这是诚实清单第 2 条的来源，别把 `calls_steady` 读成"解码循环内的分配"。

#include <cstdlib>
#include <new>

#include "video/video_plane/core/metrics/alloc_probe.hpp"

// 只在 POSIX 上编译：对齐分配用 `posix_memalign`，它的返回值归 `free` 管，
// 与 `malloc` 同源。Windows 的 `_aligned_malloc` 必须配 `_aligned_free`，
// 而 `operator delete` 拿不到"这块是哪边来的" —— 在同一个 TU 里混两套释放路径
// 迟早踩坏堆。所以 Windows 上这个文件**故意为空**，桌面只跑单测、不测这条尺子。
#if defined(__ANDROID__) || defined(__linux__)

namespace {

/// 登记"钩子装上了"。没有这一步，`calls_steady = 0` 就分不清
/// "真的零分配"和"钩子根本没链进来"—— 两者在证据里一模一样。
struct HookRegistrar {
    HookRegistrar() noexcept { kivo::video::core::metrics::alloc_probe::note_hook_installed(); }
};
const HookRegistrar g_hook_registrar;

[[nodiscard]] void* raw_allocate(std::size_t size) noexcept {
    // `malloc(0)` 可以返回 nullptr，而 `operator new(0)` 必须返回唯一指针。
    return std::malloc(size == 0 ? 1 : size);
}

[[nodiscard]] void* aligned_allocate(std::size_t size, std::size_t alignment) noexcept {
    if (alignment < sizeof(void*)) {
        return raw_allocate(size);
    }
    void* pointer = nullptr;
    // `posix_memalign` 配 `free` 是合法的一对；不用 `aligned_alloc`，
    // 因为 C11 要求 size 是 alignment 的整数倍，而 `operator new` 没这个约定。
    if (posix_memalign(&pointer, alignment, size == 0 ? alignment : size) != 0) {
        return nullptr;
    }
    return pointer;
}

}  // namespace

// ── 分配（抛异常版）──
void* operator new(std::size_t size) {
    kivo::video::core::metrics::alloc_probe::note_call(static_cast<std::int64_t>(size));
    if (void* pointer = raw_allocate(size)) {
        return pointer;
    }
    throw std::bad_alloc();
}

void* operator new[](std::size_t size) {
    kivo::video::core::metrics::alloc_probe::note_call(static_cast<std::int64_t>(size));
    if (void* pointer = raw_allocate(size)) {
        return pointer;
    }
    throw std::bad_alloc();
}

// ── 分配（nothrow 版）──
void* operator new(std::size_t size, const std::nothrow_t&) noexcept {
    kivo::video::core::metrics::alloc_probe::note_call(static_cast<std::int64_t>(size));
    return raw_allocate(size);
}

void* operator new[](std::size_t size, const std::nothrow_t&) noexcept {
    kivo::video::core::metrics::alloc_probe::note_call(static_cast<std::int64_t>(size));
    return raw_allocate(size);
}

// ── 对齐分配（C++17）──
void* operator new(std::size_t size, std::align_val_t alignment) {
    kivo::video::core::metrics::alloc_probe::note_call(static_cast<std::int64_t>(size));
    if (void* pointer = aligned_allocate(size, static_cast<std::size_t>(alignment))) {
        return pointer;
    }
    throw std::bad_alloc();
}

void* operator new[](std::size_t size, std::align_val_t alignment) {
    kivo::video::core::metrics::alloc_probe::note_call(static_cast<std::int64_t>(size));
    if (void* pointer = aligned_allocate(size, static_cast<std::size_t>(alignment))) {
        return pointer;
    }
    throw std::bad_alloc();
}

void* operator new(std::size_t size, std::align_val_t alignment, const std::nothrow_t&) noexcept {
    kivo::video::core::metrics::alloc_probe::note_call(static_cast<std::int64_t>(size));
    return aligned_allocate(size, static_cast<std::size_t>(alignment));
}

void* operator new[](std::size_t size, std::align_val_t alignment,
                     const std::nothrow_t&) noexcept {
    kivo::video::core::metrics::alloc_probe::note_call(static_cast<std::int64_t>(size));
    return aligned_allocate(size, static_cast<std::size_t>(alignment));
}

// ── 释放：一律 `free`。`posix_memalign` 与 `malloc` 的返回值都归 `free` 管 ──
void operator delete(void* pointer) noexcept { std::free(pointer); }

void operator delete[](void* pointer) noexcept { std::free(pointer); }

void operator delete(void* pointer, std::size_t) noexcept { std::free(pointer); }

void operator delete[](void* pointer, std::size_t) noexcept { std::free(pointer); }

void operator delete(void* pointer, const std::nothrow_t&) noexcept { std::free(pointer); }

void operator delete[](void* pointer, const std::nothrow_t&) noexcept { std::free(pointer); }

void operator delete(void* pointer, std::align_val_t) noexcept { std::free(pointer); }

void operator delete[](void* pointer, std::align_val_t) noexcept { std::free(pointer); }

void operator delete(void* pointer, std::size_t, std::align_val_t) noexcept {
    std::free(pointer);
}

void operator delete[](void* pointer, std::size_t, std::align_val_t) noexcept {
    std::free(pointer);
}

void operator delete(void* pointer, std::align_val_t, const std::nothrow_t&) noexcept {
    std::free(pointer);
}

void operator delete[](void* pointer, std::align_val_t, const std::nothrow_t&) noexcept {
    std::free(pointer);
}

#endif  // __ANDROID__ || __linux__
