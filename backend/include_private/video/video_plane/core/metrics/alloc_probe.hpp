#pragma once

#include <array>
#include <atomic>
#include <cstdint>

namespace kivo::video::core::metrics {

/// 进程级堆分配计数器（P14-A / G3）。
///
/// 为什么需要它：G3 的判据是"稳态循环内 `operator new` 调用数 = 0"。
/// 用 `mallinfo` 只能看堆涨了多少 —— 一个反复 new/delete 但总量不变的热路径
/// 在它眼里是干净的。判据要的是**调用次数**，所以只能替换分配器。
///
/// 分工：本文件**只做计数与开关**，替换 `operator new` 的是
/// `alloc_probe_hook.cxx`。拆开是为了让桌面单测能引用计数器而不被迫替换分配器。
///
/// 线程安全：全部是 relaxed 原子。计数只求总量正确，不参与任何顺序判断 ——
/// 用 seq_cst 只会让每次分配多一条栅栏，在 60fps 的热路径上是白花的时间。
namespace alloc_probe {

/// 允许计数（`operator new` 在未启用时仍然转发，只是不计数）。
///
/// 为什么要开关而不是"一直数"：进程启动、JNI 初始化、字符串常量池都在分配，
/// 把它们算进"稳态"会让 G3 那个 0 永远不可能成立。
void arm() noexcept;
void disarm() noexcept;

/// 标记"稳态开始"（首帧渲染之后）。
///
/// 与 `arm()` 分开的理由：`arm()` 在会话 start 时调用，那时还在开解码器、
/// 建 Surface、扩容 vector；那些分配是启动成本，不是稳态。G3 判的是后者。
void mark_steady() noexcept;

[[nodiscard]] bool armed() noexcept;
[[nodiscard]] std::int64_t calls_total() noexcept;
[[nodiscard]] std::int64_t calls_steady() noexcept;
[[nodiscard]] std::int64_t bytes_total() noexcept;
[[nodiscard]] std::int64_t bytes_steady() noexcept;

/// 临时挂起 / 恢复计数。**不碰 `steady` 标记**（`arm()` 会把它清掉）。
///
/// 用在哪：`DecodeSession::snapshot()` 自己会复制一堆 `std::string` ——
/// 那是**测量**在分配，不是**播放**在分配。第一版没挂起，于是 UI 每秒读 15 次统计，
/// 计数器就报"稳态分配 15 次/秒"，看起来像热路径在乱分配，实际是尺子在量自己。
void suspend() noexcept;
void resume() noexcept;

/// 归零并关闭。下一次 `arm()` 重新开始。
void reset() noexcept;

/// 供 `alloc_probe_hook.cxx` 调用的记账入口。**不是给业务代码用的。**
void note_call(std::int64_t size) noexcept;

/// 由替换实现（`alloc_probe_hook.cxx`）在静态初始化时调用一次。
///
/// 为什么需要一个"钩子装上了没有"的布尔：`calls_steady = 0` 有两种可能 ——
/// **真的零分配**，或者**根本没装钩子**。这两件事在证据里长得一模一样，
/// P13-T 就因为 `drift_rebases` 漏写吃过一次（"门从没触发" vs "门没接上"）。
void note_hook_installed() noexcept;
[[nodiscard]] bool hook_installed() noexcept;

}  // namespace alloc_probe

/// 一个线程的分配账（P14-B）。
///
/// `calls/bytes` 是该线程**累计**值；`calls_steady` 的线程分解不在结构里，
/// 因为稳态起点对每个线程都是同一刻（首帧渲染），减一次就够，不必存两份。
struct ThreadAllocFacts {
    std::int32_t tid = 0;
    std::int64_t calls = 0;
    std::int64_t bytes = 0;
    /// 首帧渲染之后的调用数。**G3 的正主**：进程总量里混着 UI 轮询，
    /// 只有按线程分开，才能说"播放线程是不是零分配"。
    std::int64_t calls_steady = 0;
    std::int64_t bytes_steady = 0;
    /// 内核 `comm`（最长 15 字节）。**空不等于没有线程** —— 未命名的线程就是空。
    char name[24] = {};
};

/// 槽表容量。8 个够用：播放会话里常驻的线程是 recv / submit / audio / poll / main。
///
/// 为什么是定长数组而不是 `unordered_map<tid, …>`：那本身要分配，
/// 而我们正在 `operator new` 里面 —— 递归。定长数组是唯一安全的选择。
inline constexpr std::int32_t kMaxTrackedThreads = 8;

/// 进证据的分配事实。**事实与结论分开**：这里只有计数，不判"是否为零"。
///
/// 放在 `alloc_probe` 之外：它是**证据形状**，不是探针的接口 ——
/// 探针只需要 `arm/note/mark`，读证据的渲染器只需要这个结构。
struct AllocFacts {
    bool hook_installed = false;  ///< 替换实现是否真的链进来了
    bool armed = false;           ///< 是否处于计数区间
    std::int64_t calls_total = 0;
    std::int64_t calls_steady = 0;  ///< 首帧渲染之后的调用数 —— G3 判的是这个
    std::int64_t bytes_total = 0;
    std::int64_t bytes_steady = 0;

    /// 按线程分解（P14-B）。这是"5 次/秒到底是谁"的唯一答案来源。
    std::array<ThreadAllocFacts, kMaxTrackedThreads> threads{};
    std::int32_t thread_count = 0;

    /// 槽表满了之后落到"溢出"槽的调用数。
    ///
    /// 必须出版：没有它，"只有 8 个线程分配过"和"第 9 个线程之后全丢了"
    /// 在证据里长得一模一样。
    std::int64_t overflow_calls = 0;
};

[[nodiscard]] AllocFacts make_alloc_facts() noexcept;

}  // namespace kivo::video::core::metrics
