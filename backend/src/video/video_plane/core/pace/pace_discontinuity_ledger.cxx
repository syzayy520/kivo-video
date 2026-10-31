#include "video/video_plane/core/pace/pace_discontinuity_ledger.hpp"

namespace kivo::video::core::pace {
namespace {

/// 环最多留几笔。与 `RebaseTrace` 同一个数、同一个理由：够看出形状，不随会话变长。
constexpr std::int32_t kRingSize = 8;

}  // namespace

void DiscontinuityLedger::note(DiscontinuityKind kind, std::int64_t at_ns,
                               std::int64_t wall_delta_ns, std::int64_t raw_delta_ns,
                               std::int64_t step_ns, std::int64_t lead_before_ns,
                               std::int64_t lead_after_ns) noexcept {
    DiscontinuitySample sample{};
    sample.at_ns = at_ns;
    sample.kind = kind;
    sample.wall_delta_ns = wall_delta_ns;
    sample.raw_delta_ns = raw_delta_ns;
    sample.step_ns = step_ns;
    sample.lead_before_ns = lead_before_ns;
    sample.lead_after_ns = lead_after_ns;

    ++events_;
    switch (kind) {
        case DiscontinuityKind::kExtrapolated:
            ++extrapolated_;
            break;
        case DiscontinuityKind::kMonotonicRejected:
            ++rejected_;
            break;
        case DiscontinuityKind::kBackwardClamped:
            ++clamped_;
            break;
        case DiscontinuityKind::kJumpRebased: {
            ++jumped_;
            // 跳变分支把台阶**丢掉**了（`position += wall_delta`），`step` 恒 0 ——
            // "被丢掉多少"只能从原始量算：`raw_delta − wall_delta`。
            // 带符号保留方向；只在**绝对值**更大时替换，因为两笔反向跳变加起来会互相抵消。
            const std::int64_t discarded_ns = raw_delta_ns - wall_delta_ns;
            const std::int64_t abs_discarded = discarded_ns < 0 ? -discarded_ns : discarded_ns;
            const std::int64_t abs_kept = jump_max_ns_ < 0 ? -jump_max_ns_ : jump_max_ns_;
            if (jumped_ == 1 || abs_discarded > abs_kept) {
                jump_max_ns_ = discarded_ns;
            }
            break;
        }
        case DiscontinuityKind::kPositionStep:
            ++stepped_;
            break;
        case DiscontinuityKind::kNone:
            // 不该发生：调用方只在真事件上记账。记成 `events_` 增一而分类不动，
            // 判据 Y2 立刻红 —— 这比在这里静默吞掉更容易查。
            break;
    }
    if (events_ == 1) {
        first_at_ns_ = at_ns;
    }

    offset_ns_ += step_ns;

    // `max` 按**绝对值**取：台阶可以是负的（钟停住、位置落后），
    // 只比大小会让一笔 −300 ms 输给 +5 ms。
    const std::int64_t abs_step = step_ns < 0 ? -step_ns : step_ns;
    const std::int64_t abs_max = max_.step_ns < 0 ? -max_.step_ns : max_.step_ns;
    if (events_ == 1 || abs_step > abs_max) {
        max_ = sample;
    }

    // 环满时先把自己要挤掉那一笔从"环内累计"里扣回来，再写新的。
    // 不做这一步，`sampled_offset_ns` 会变成"历史总和"——那它和 `offset_ns` 就没区别了。
    if (count_ == kRingSize) {
        sampled_offset_ns_ -= recent_[head_].step_ns;
    }
    recent_[head_] = sample;
    head_ = (head_ + 1) % kRingSize;
    if (count_ < kRingSize) {
        ++count_;
    }
    sampled_offset_ns_ += step_ns;
}

DiscontinuityTrace DiscontinuityLedger::trace() const noexcept {
    DiscontinuityTrace out{};
    out.events = events_;
    out.extrapolated = extrapolated_;
    out.rejected = rejected_;
    out.clamped = clamped_;
    out.jumped = jumped_;
    out.stepped = stepped_;
    out.offset_ns = offset_ns_;
    out.sampled_offset_ns = sampled_offset_ns_;
    out.sampled = count_;
    out.samples_used = count_;
    out.first_at_ns = first_at_ns_;
    out.jump_max_ns = jump_max_ns_;
    out.max = max_;
    // 环按时间升序展开：`head_` 是"下一笔写到哪"，写满之后它正好是最旧那一笔。
    const std::int32_t oldest = count_ < kRingSize ? 0 : head_;
    for (std::int32_t i = 0; i < count_; ++i) {
        out.recent[i] = recent_[(oldest + i) % kRingSize];
    }
    return out;
}

void DiscontinuityLedger::reset() noexcept {
    events_ = 0;
    extrapolated_ = 0;
    rejected_ = 0;
    clamped_ = 0;
    jumped_ = 0;
    stepped_ = 0;
    offset_ns_ = 0;
    sampled_offset_ns_ = 0;
    first_at_ns_ = 0;
    jump_max_ns_ = 0;
    max_ = DiscontinuitySample{};
    for (DiscontinuitySample& sample : recent_) {
        sample = DiscontinuitySample{};
    }
    head_ = 0;
    count_ = 0;
}

}  // namespace kivo::video::core::pace
