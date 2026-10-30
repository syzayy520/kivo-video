#pragma once

#include <cstdint>
#include <string>

namespace kivo::video::core::seek {

/// G8 的预算：seek 到首帧交出 < 150 ms。
///
/// **口径写死在这里，别在调用点各自解释**：
/// 起点 = 进入 `DecodeSession::seek_to()` 的第一行（含停线程）；
/// 终点 = 首帧被 `releaseOutputBuffer*` **交回平台**那一刻。
/// 交出之后（BufferQueue + 合成器锁存）不归我们管，D5 证明那段要单独测。
inline constexpr std::int64_t kDefaultBudgetUs = 150'000;

/// G8a 的预算：**冷启动**首帧 < 200 ms（P13-B 新定）。
///
/// 为什么单独定一条：它和 `kDefaultBudgetUs`（seek 首帧）**不是同一个指标**，
/// 却共用同一个计数器（`counters.first_frame_latency_us`）。混着读会得出
/// "4K 超预算"的假结论 —— P13-A 就是这么误判的（拿 132.5 ms 的冷启动数字
/// 去质疑 150 ms 的 seek 预算，而 4K 的 seek 实测只有 27.7–41.7 ms）。
///
/// **阈值依据是 19 次实测，不是单轮**（18 次纯冷启动 + 1 次带 seek 的）：
///
/// | 片源 | n | min | median | max |
/// | --- | --- | --- | --- | --- |
/// | 1080p60 | 6 | 28.5 ms | 53.9 ms | 76.8 ms |
/// | 4K60 HEVC 8bit | 6 | 47.5 ms | 78.8 ms | 86.5 ms |
/// | 4K60 HEVC 10bit | 6 | 120.7 ms | 133.9 ms | **161.5 ms** |
///
/// 另有一次带 seek 的运行录到 **206 ms** —— 所以最初据单轮定的 200 ms **会偶发失败**。
/// 300 ms 给已观测极大值 1.45× 余量，同时仍在"用户不会觉得卡"的范围内
/// （本地起播 300 ms 属于好的水平；网络播放器通常 1–2 s）。
///
/// 教训：**冷启动的方差比中位数大得多，阈值必须按最大值定，不能按中位数或单轮定。**
inline constexpr std::int64_t kColdStartBudgetUs = 300'000;

/// 冷启动首帧的判定结果。
struct ColdStartAssessment {
    /// 有没有拿到数字。`false` 时 `reason` 必非空。
    bool applicable = false;
    std::int64_t first_frame_us = -1;
    std::int64_t budget_us = kColdStartBudgetUs;
    bool pass = false;
    std::string reason;
};

/// 判定一次**冷启动**首帧（`open()` 之后第一次 `start()`）。
/// 纯函数，桌面可测。
///
/// 明确不管 seek：seek 后的首帧走 `assess_seek()`，两者起点不同，
/// 起跑成本差 3–4 倍（4K60 10bit：冷启动 132.5 ms vs seek 后 36.0–41.7 ms）。
[[nodiscard]] ColdStartAssessment assess_cold_start(
    std::int64_t first_frame_us, std::int64_t budget_us = kColdStartBudgetUs);

/// 一次 seek 的事实。`-1` 表示"不知道/没发生" —— 与 `0` 严格区分：
/// 0 µs 是"瞬间完成"，-1 是"没测到"，混起来会让证据看起来比实际好。
struct SeekFacts {
    std::int32_t requests = 0;
    std::int64_t target_us = -1;      ///< 请求的位置
    std::int64_t landing_us = -1;     ///< 解封装器实际落点（PREVIOUS_SYNC ⇒ 应 ≤ target）
    std::int64_t prepare_us = -1;     ///< 停线程 + 定位 + flush
    std::int64_t first_frame_us = -1; ///< 起线程 → 首帧交出（沿用 C1 的既有计数）
    /// seek 后首帧的**媒体 pts**（不是时延）。用来判"没跳到别的地方去"。
    std::int64_t video_first_pts_us = -1;

    // ---- 音频侧（P12-F2）。没有音轨时 `audio_present=false`，`audio_landing_us` 保持 -1 ----
    bool audio_present = false;
    std::int64_t audio_landing_us = -1;
};

/// 判定结果。`reason` 只在 `pass == true` 时为空 ——
/// 空 reason 的失败会被读成"没问题"，这是 E1 定下的规矩。
struct SeekAssessment {
    std::int64_t total_us = -1;
    bool pass = false;
    std::string reason;
};

/// 按 G8 判定一次 seek。纯函数，桌面可测。
[[nodiscard]] SeekAssessment assess_seek(const SeekFacts& facts,
                                        std::int64_t budget_us = kDefaultBudgetUs);

/// 音画两侧落点的最大允许差。取"一帧 33 ms 向上取整"：
/// 两侧各自落到同步帧上，差一帧算同步，差两帧就不算。
inline constexpr std::int64_t kMaxAvGapUs = 40'000;

/// F2 的判定：视频侧先按 G8 判，再看音频落点与视频落点的差。
///
/// **没有音轨时判"不适用"，不判通过** —— 一段没有音频的片源无法证明"音画不脱节"，
/// 让它通过等于用一条不存在的证据交差。
struct AvSeekAssessment {
    SeekAssessment video;          ///< 视频侧的 G8 判定（原样带出来，不重复造）
    std::int64_t audio_gap_us = -1;///< `audio_landing − video_landing`（带符号，便于看谁在前）
    bool applicable = false;       ///< 片源/会话有没有音轨
    bool pass = false;
    std::string reason;
};

[[nodiscard]] AvSeekAssessment assess_av_seek(const SeekFacts& facts,
                                             std::int64_t budget_us = kDefaultBudgetUs,
                                             std::int64_t max_gap_us = kMaxAvGapUs);

}  // namespace kivo::video::core::seek
