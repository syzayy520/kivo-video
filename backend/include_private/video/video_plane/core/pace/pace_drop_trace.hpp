#pragma once

#include <cstdint>

namespace kivo::video::core::pace {

/// 一帧被丢掉时的现场（P14-AG）。
///
/// 只记两个数，因为要回答的问题只有一个：**"丢在哪几秒"**。
/// `media_ns` 就是那个坐标系 —— 与 `DriftExtreme.media_ns` 同一个选择：
/// 单调钟在不同会话里差着开机时长，列出来没法读；媒体时刻能直接和片长对照。
struct DropSample {
    /// 被丢掉那一帧的 pts。
    std::int64_t media_ns = 0;
    /// 丢它的时候迟到多少（判据是 `late_drop_ns`，默认 100 ms）。
    std::int64_t lateness_ns = 0;
};

/// 丢掉的那几帧在哪儿 —— 定长环 + 两个计数。
///
/// **为什么需要它**：`pace.dropped` 只是一个计数。一份证据说"丢了 24 帧"，
/// 却没有任何现场回答"它们是注入后 100 ms 内一次性丢的，还是分散在整段里"——
/// 这两种形状对判读完全相反（前者是台阶被跳掉，后者是基准一直在飘）。
/// 这就是 P14-Y 花一整卡消灭的那种"沉默的计数"，只不过这次在丢帧这一侧。
struct DropTrace {
    /// 一共丢了多少帧（**不受环长限制**）。
    std::int64_t total = 0;
    /// 环里留下几笔。
    std::int64_t sampled = 0;
    /// `true` ⇒ 丢的比环多，`recent` 里只是**最后** 16 笔。
    bool truncated = false;
    /// 按时间升序展开。
    DropSample recent[16]{};
};

/// 环本体：热路径上只有一次比较与一次写，无分配。
class DropRing {
public:
    /// 环最多留几笔。取 16 而不是 8：P14-AD 的 800 ms 那轮丢了 24 帧，
    /// 8 格会把"一次性丢"的形状截成半个；16 格能看出它是一簇还是两簇。
    static constexpr std::int32_t kCapacity = 16;

    void note(std::int64_t media_ns, std::int64_t lateness_ns) noexcept {
        ++total_;
        recent_[head_] = DropSample{media_ns, lateness_ns};
        head_ = (head_ + 1) % kCapacity;
        if (count_ < kCapacity) {
            ++count_;
        }
    }

    [[nodiscard]] DropTrace trace() const noexcept {
        DropTrace out{};
        out.total = total_;
        out.sampled = count_;
        out.truncated = total_ > kCapacity;
        // 写满之后 `head_` 正好指向最旧那一笔。
        const std::int32_t oldest = count_ < kCapacity ? 0 : head_;
        for (std::int32_t i = 0; i < count_; ++i) {
            out.recent[i] = recent_[(oldest + i) % kCapacity];
        }
        return out;
    }

    void reset() noexcept {
        total_ = 0;
        head_ = 0;
        count_ = 0;
        for (DropSample& sample : recent_) {
            sample = DropSample{};
        }
    }

private:
    std::int64_t total_ = 0;
    DropSample recent_[kCapacity]{};
    std::int32_t head_ = 0;
    std::int32_t count_ = 0;
};

}  // namespace kivo::video::core::pace
