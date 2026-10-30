#include "video/video_plane/core/audio/clock/clock_offset_source.hpp"

namespace kivo::video::core::audio {

ClockOffsetSource::ClockOffsetSource(AudioClockSource& inner, std::int64_t offset_ns,
                                     std::int64_t arm_after_ns,
                                     std::int32_t rate_ppm) noexcept
    : inner_(inner),
      offset_ns_(offset_ns),
      arm_after_ns_(arm_after_ns < 0 ? 0 : arm_after_ns),
      rate_ppm_(rate_ppm) {}

AudioClockSnapshot ClockOffsetSource::now() {
    AudioClockSnapshot snapshot = inner_.now();

    if (!has_origin_) {
        has_origin_ = true;
        origin_position_ns_ = snapshot.position_ns;
    }

    // 陈旧读数不动：那一段位置本来就是冻结值，在冻结值上叠台阶会让
    // "时钟停更"和"时钟跑偏"两件事混在同一份证据里。
    if (snapshot.stale) {
        return snapshot;
    }

    if (!applied_ && snapshot.position_ns - origin_position_ns_ >= arm_after_ns_) {
        applied_ = true;
        armed_at_position_ns_ = snapshot.position_ns;
        rate_origin_position_ns_ = snapshot.position_ns;
    }

    // P13-AD：速率误差。真实事件全是这个形状（8 秒里从 0 慢慢长到 73–165 ms），
    // 而前两张卡注入的都是瞬时台阶 —— 两者对门的要求不一样：
    // 台阶考验"当帧还多少"，速率考验"能不能一直追住、不越攒越多"。
    //
    // 倍率从注入点起算：`pos' = origin + (pos − origin) × (1 + ppm/1e6)`。
    // 从注入点而不是从 0 起算，是因为前面的样本不该被改写 —— 否则
    // `first_lateness_ns`、跨度账、`arm_after` 的语义全部跟着漂。
    if (applied_ && rate_ppm_ != 0) {
        const std::int64_t span_ns = snapshot.position_ns - rate_origin_position_ns_;
        // 整数运算即可：span ≤ 20 s（2e10）× ppm ≤ 1e5 ⇒ 2e15，int64 装得下。
        // （用过 `__int128`，MSVC 不认 —— 桌面构建当场编译失败。）
        snapshot.position_ns += (span_ns * rate_ppm_) / 1'000'000;
    }

    // 台阶是**持续的**偏移，不是一次性脉冲：真实时钟跑偏之后位置会一直超前，
    // 下一帧不会自己走回来。只在注入那一帧加一次的话，下一帧位置退回去会被
    // tracker 的倒退钳制顶住，于是"一次台阶"变成"一串钳制" ——
    // 那时证据里的形状不再是我们要测的东西。
    if (applied_) {
        snapshot.position_ns += offset_ns_;
    }
    return snapshot;
}

void ClockOffsetSource::rebase(std::int64_t media_position_ns) noexcept {
    inner_.rebase(media_position_ns);
    if (applied_) {
        // 已经注入过一次就**不再注入**。台阶是一次性事件，重复注入会让
        // "一次台阶后门怎么收敛"这个观测变成"两次台阶"，账再也算不清。
        return;
    }
    // 新基准 = 新的位置参照系。`origin` 留着会让 `arm_after` 立刻成立
    // （seek 之后位置可能一下子跳到几十秒），于是台阶被"顺手"注入 ——
    // 那不是我们要测的东西。
    has_origin_ = false;
    armed_at_position_ns_ = -1;
}

}  // namespace kivo::video::core::audio
