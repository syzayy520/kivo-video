#pragma once

#include <atomic>
#include <cstdint>

namespace kivo::video::mediacodec {

/// 一段平台调用的耗时事实：调用了几次、一共花了多久、最慢的一次多久。
///
/// 为什么按段分开记：C6 的全部收益来自"把两段重叠起来"，
/// 若只报总 fps，就说不清收益从哪来；而"该不该继续拆"也只能靠段耗时判断
/// （见 `19-p12c6-design.md` §6.3）。峰值单独留一格 —— 平均值会把
/// "偶尔卡 40ms"抹成"平均 2ms"，而卡顿正是用户能感知的那部分。
struct StageCost {
    std::int64_t calls = 0;
    std::int64_t total_us = 0;
    std::int64_t max_us = 0;

    [[nodiscard]] double mean_us() const noexcept {
        return calls > 0 ? static_cast<double>(total_us) / static_cast<double>(calls) : 0.0;
    }
};

/// 单段耗时的累加器。只在执行该调用的那个线程里写，任意线程可读。
///
/// 用原子量而不是锁：每帧要记两次（进/出），锁的开销会污染被测对象本身。
class StageCostMeter {
public:
    void record(std::int64_t duration_us) noexcept;
    [[nodiscard]] StageCost load() const noexcept;

private:
    std::atomic<std::int64_t> calls_{0};
    std::atomic<std::int64_t> total_us_{0};
    std::atomic<std::int64_t> max_us_{0};
};

}  // namespace kivo::video::mediacodec
