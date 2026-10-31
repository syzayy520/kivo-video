#pragma once

#include <cstdint>

#include "video/video_plane/core/audio/clock/audio_clock_types.hpp"
#include "video/video_plane/core/pace/pace_action.hpp"
#include "video/video_plane/core/pace/pace_drift_series.hpp"
#include "video/video_plane/core/pace/pace_drift_trend.hpp"
#include "video/video_plane/core/pace/pace_drift_window.hpp"
#include "video/video_plane/core/pace/pace_histogram.hpp"
#include "video/video_plane/core/pace/pace_rebase_trace.hpp"
#include "video/video_plane/core/pace/pace_lead_trace.hpp"
#include "video/video_plane/core/pace/pace_measurability.hpp"
#include "video/video_plane/core/pace/pace_wall_offset.hpp"
#include "video/video_plane/core/pace/pace_worst_episode.hpp"

namespace kivo::video::core::pace {

/// 一帧的「目标时刻偏差」（P13-P）。
///
/// 单独一个结构而不是给 `note_presented()` 再加两个裸 `int64_t`：这两个数必须同来同往，
/// 分开传就会有人只给一个 —— 而只给 `due_step` 不给帧长时，"超没超一帧"无法判定，
/// 那个计数会静静地变成错的。
struct DueStep {
    /// `due_at − (上一帧 due_at + 一帧)`。负 = 目标时刻被算早了（表快了）。
    std::int64_t due_step_ns = 0;
    /// 0 = 调用方没提供 ⇒ 本帧不计入 `due_step_supplied`，校验器据此抓"忘了接线"。
    std::int64_t frame_duration_ns = 0;
    /// `(present_k − present_{k−1}) − 一帧`（P13-Q）。
    ///
    /// 与 `due_step_ns` 是**两个独立测量的量**：这个来自交出时刻的时间戳（也就是
    /// `present_jitter` 的样本），那个来自时钟算出的目标时刻。恒等式
    /// `Δlateness = present_step − due_step` 因此是有内容的，不是同义反复。
    ///
    /// ⚠️ 它必须留在第 3 位。P13-R 加钟读数时插到了它前面，`DueStep{a, b, present_step}`
    /// 就把 `present_step` 悄悄写进了 `clock_mono_ns` —— 编译通过、单测打红。
    /// 位置本身就是约定，插字段只能往后加。
    std::int64_t present_step_ns = 0;

    /// 本帧读取时钟的三个原始量（P13-R）。`due = mono + (pts − anchor_pts − elapsed)`
    /// 是 `arrive()` 正常分支的定义式，出版这三格是为了让 `due_step` 的来源可复算：
    /// 是**钟**走快了（`elapsed` 相对 `mono` 偏大），还是**帧到得早**（`mono` 相对 `pts` 偏小）。
    std::int64_t clock_mono_ns = 0;
    std::int64_t clock_elapsed_ns = 0;
    std::int64_t anchor_pts_ns = 0;
    /// 本帧的 `due_at`，与上面三格一起满足定义式（正常分支）。
    std::int64_t due_at_ns = 0;
};

/// 一次节拍会话的度量结果。全部是事实计数与分布，不含评价。
///
/// 是否达标由证据层按 G6 判定，节拍器只负责如实记录。
struct PaceSummary {
    // ---- 决策计数 ----
    std::int64_t frames_decided = 0;
    std::int64_t presented = 0;      ///< kWait + kPresent
    std::int64_t dropped = 0;        ///< kDrop + kCatchUp

    /// **决策了、但没落地**的帧数（P14-Q）。`= frames_decided − presented − dropped`。
    ///
    /// 上面三格记的是**决策**，但 `kWait` / `kPresent` 之后还有一段路：
    /// release → Surface → 真正显示。那段路上有两种结局没被 `presented` 记：
    ///   ① 换面空窗期（`render` 被强制置 false）—— 已由
    ///      `DecodeStats::frames_discarded_no_surface` 单独记，**合法**；
    ///   ② 会话在交付之前结束（屏幕熄灭、切后台、异常退出）—— 没有计数器。
    /// 两种在报告里长得一样，都是 `presented + dropped < frames_decided`。
    ///
    /// 这一格把那个差**印出来**，不让读者自己在脑子里减。
    /// 它非 0 时只有一个合法的读法：**等于 `frames_discarded_no_surface`**
    /// （判据 S2）。不等就是②——有帧连去向都没有。
    ///
    /// 真机反例（P14-P 取证事故）：`frames_decided = 1800 / presented = 84 /
    /// dropped = 0`，差额 1716 帧在 `/27` 的报告里**不占任何一格**。
    ///
    /// **不带 `measured`**：这不是测量，是三个计数器的差。给一个纯派生量
    /// 配"我很确定"，是 P14-L 反的那类东西（见 `147-p14q-design.md` §3.2）。
    std::int64_t undelivered_frames = 0;
    std::int64_t catch_up_frames = 0;
    std::int64_t catch_up_events = 0;
    std::int64_t waited = 0;
    std::int64_t wait_total_ns = 0;

    // ---- 时钟健康度 ----
    std::int64_t extrapolated_frames = 0;
    std::int64_t free_run_frames = 0;
    std::int64_t jump_rebases = 0;
    std::int64_t stale_readings = 0;
    std::int64_t backward_clamps = 0;
    std::int64_t monotonic_rejects = 0;
    /// 累积漂移重锚的帧数（P13-S）。与 `jump_rebases` 分开：那个是"跳了 1 秒"，
    /// 这个是"每帧只差一点、攒到 45 ms"。非 0 说明这一轮的钟**速率**不对。
    std::int64_t drift_rebases = 0;
    /// 单帧修正的最大幅度与累计量（P13-AC）。
    ///
    /// 只数次数分不出"一帧削平"和"二十三帧摊平"：P13-AB 是前者（单帧 45 ms，
    /// 于是事件本身变成 29–31 ms 的迟到尖峰），P13-AC 之后应是后者
    /// （单帧 ≤ 2 ms）。这两个数就是那件事的唯一判据。
    std::int64_t drift_max_correction_ns = 0;
    std::int64_t drift_corrected_total_ns = 0;

    /// 每次复位的时刻与幅度（P14-X）。由 `FramePacer::summary()` 从 tracker 抄进来 ——
    /// `PaceStats` 自己没有墙钟原点，它只有"第几帧"。**不在这里算**：
    /// 算两次就是两个真相（`frame_worst` 与 `episode_` 的老账，P13-P）。
    RebaseTrace drift_rebase_trace{};

    // ---- 门本身的配置（P13-AD）----
    //
    // 出版它，是为了让判据不必硬编码 45 / 5：校验器拿证据里的数去算，
    // 改策略时只升 schema，不会出现"新实现被旧上限卡住"。
    // `drift_gate_enabled == false` 的轮次是对照组，它的 `drift_rebases = 0`
    // **不可读作"门判断沉默"** —— 那是"门被关掉了"。
    bool drift_gate_enabled = true;
    std::int64_t drift_limit_ns = 0;
    std::int64_t drift_band_ns = 0;
    ClockHealth health = ClockHealth::kFresh;
    const char* clock_source = "unknown";

    // ---- 抖动分布 ----
    /// `present_at - due_at`（有符号，早为负）
    SignedHistogram lateness{};
    /// `(present[i] - present[i-1]) - frame_duration`
    SignedHistogram interval{};
    /// 被丢弃帧的迟到量。与 `lateness` 分开：混在一起会把 p99 抬到"丢帧的迟到"上，
    /// 而 G6 关心的是**实际呈现**的抖动。
    SignedHistogram dropped_lateness{};

    /// `submit_at - due_at`（P12-C7 的 `kSurfaceAtTime` 路径）：提交时刻相对目标显示时刻的偏移。
    ///
    /// 与 `lateness` 分开的理由：`lateness` 是"睡到点之后迟了多少"，正常应 ≈ 0；
    /// 而 `schedule_lead` 是"提前多久交出去"，正常应是**负的固定值**（提前量）。
    /// 两者混在一起会让 `over_budget_frames` 把"正常提前"判成"超预算"。
    ///
    /// 判稳定性看**跨度**（p1 到 p99），不看均值 —— 均值大不代表不稳。
    SignedHistogram schedule_lead{};

    /// 实际进了直方图的呈现样本数。小于 `presented` 说明调用方忘了回填时刻。
    std::int64_t latency_samples = 0;

    // ---- 抽稀口径（P13-D）----
    //
    // `lateness` 是**逐帧**分布：它的 p99 随帧率变（60fps 的样本密度是 30fps 的两倍），
    // 同一台机器读出"60fps 更差"。所以证据层还要一份**按媒体时间抽稀到 20 Hz** 的分布，
    // 它的 p99 才跨帧率可比。两份都留着：G6 的 `over_budget_frames` 必须逐帧数，
    // 抽稀会漏掉恰好落空的那几帧。
    SignedHistogram sampled_lateness{};
    std::int64_t sampled_samples = 0;
    std::int64_t drift_observed_frames = 0;  ///< 喂给抽稀门的帧数
    std::int64_t drift_skipped_frames = 0;   ///< 被门拒掉的帧数
    /// 没有帧跨过的格点数（播放空洞）。补样本等于编数据，所以只报不补。
    std::int64_t drift_unobserved_cells = 0;
    /// **整场**媒体时间跨度（各段之和，P13-H）。`sampled_samples` 的分母。
    ///
    /// 与 `drift.span_ns`（窗口跨度，封顶 102.35 s）不是一回事：
    /// 拿窗口跨度去除累计样本数，超过 102.4 s 之后商随片长线性增长。
    /// 两者在 8 s / 35 s 片源上相等，缺陷只在长片源上现形。
    std::int64_t drift_media_span_ns = 0;
    /// **本段时间轴**内被接受的样本数（P13-I）。`segments > 1` 时小于
    /// `sampled_samples`，差额是"被 seek 丢掉的那几段"，不是"被环形覆盖的"。
    ///
    /// 与 `sampled_samples` 并列摆出来，是为了让"窗口里的点为什么比累计的少"
    /// 有一个能查的答案 —— 上一版只有 `truncated` 一个字段，于是所有减少
    /// 都被读成覆盖（见 `72-p13i-design.md` §一）。
    std::int64_t drift_segment_samples = 0;
    /// **本段内**最宽的空洞（格点，P13-M）。0 = 段内没有整格丢失。
    ///
    /// 与 `drift_unobserved_cells` 是两个问题：那个是整场累计（各段相加），
    /// 这个是当前段的最大值。累计值答不出"最宽的洞有多宽"，而趋势的可信度
    /// 取决于最大值 —— 十个 100 ms 的小洞不会让一条直线失去意义，一个 10 s 的会。
    std::int32_t drift_segment_largest_gap_cells = 0;
    /// 本段空洞没有超过 `DriftSeries::kMaxContiguousGapCells`（20 格 = 1 s）。
    ///
    /// 为 false 时 `drift_media_span_ns` 与由它算出的 `observed_hz` **包含**那个洞，
    /// 不是"实际走过的时长"。这条限制不靠读者去查设计稿，就写在字段旁边。
    bool drift_segment_contiguous = true;

    /// 漂移趋势（窗口中位数 + 两道斜率）。短片段会带 `usable = false` 与非空 reason。
    DriftTrend drift{};

    /// 抽稀口径的最差点（P13-F）—— 由 `DriftSeries` 累计维护，`of` = `sampled_samples`。
    DriftExtreme sampled_worst{};

    /// **窗口**口径的迟到分布（P13-I）。样本 = 序列里还留着的那些点。
    ///
    /// 与 `sampled_lateness` 并列而不是替掉它：两者回答两个问题 ——
    /// "整场有多差"（累计）与"趋势描述的那一段有多差"（窗口）。
    /// 未覆盖且单段时它们逐桶相同（判据 I4 会核对这件事）；一旦不同，
    /// `truncated` / `segments` 里必有其一能解释差异。
    DriftWindow window_lateness{};

    /// 逐帧口径的最差点（P13-F）。`of` = `lateness.count()`。
    ///
    /// 逐帧**没有序列**（只有直方图），所以极值由 `note_presented()` 里 O(1) 比较维护：
    /// 那里是唯一同时握着 `lateness_ns` 与 `media_ns` 的地方。
    /// 想给它也养一条 2048 点序列来算极值，是拿 32 KB 换一次比较。
    ///
    /// P13-P 起**由 `frame_episode` 派生**（同一次比较既挑最差也记前后邻帧），
    /// 不再单独维护 —— 两个地方写同一个极值，早晚一个说 88.231、一个说 88.230。
    DriftExtreme frame_worst{};

    /// 最差帧的前因后果（P13-P）：`due_step` + 前 2 帧 + 后 8 帧。
    ///
    /// 与 `frame_worst` 是同一帧的两面：那个回答"在哪、多大"，这个回答"是手慢了还是表快了"。
    /// 之所以两个都出版：`frame_worst` 的四个字段已经被 13 份老证据引用（判据 F1 在守它），
    /// 把 `worst` 块改成新结构会让那些证据无法对读。
    WorstEpisode frame_episode{};

    /// `|due_step| > frame_duration` 的呈现帧数（P13-P）。
    ///
    /// **一行摘要**：非零即说明这一轮的"时间基准"动过 —— 与渲染快慢无关。
    /// 4K 尖峰那张卡的旧证据里它就是 6（`p13d-4k60-r1`），而 `present_jitter` 全程 ±20 ms。
    std::int64_t due_step_frames = 0;
    /// 有 `due_step` 输入的呈现帧数。小于 `presented` ⇒ 调用方漏接了这一路参数，
    /// 此时 `due_step_frames` 的 0 **不可读作"没发生位移"**。校验器 P5 守这条。
    std::int64_t due_step_supplied = 0;

    // ---- 两侧的累积（P13-Q）----
    //
    // 单帧的 `due_step` 只有"一步"的量级；缓慢位移每步 ≤ 一帧，`due_step_frames`
    // （门限一帧）会记 0，于是 `p13d-4k60-r1` 那种"88 → 5 用六帧"的形状在 P13-P 的
    // 字段里是隐形的。这三个数把整段账记下来：
    //
    //     last_lateness − first_lateness == sum_present_step − sum_due_step
    //
    // 与单帧恒等式不同，这一条**跨帧**成立，两侧又各自独立测量，所以它能抓到
    // "每步都很小、累积很大"的位移。
    std::int64_t first_lateness_ns = 0;
    std::int64_t last_lateness_ns = 0;
    std::int64_t sum_present_step_ns = 0;
    std::int64_t sum_due_step_ns = 0;
    /// 上面那条恒等式的残差（ns）。0 = 模型自洽；非 0 必有中间帧没被记账
    /// （丢帧、锚点重建、`present_step` 漏传），此时累积账不可用。
    std::int64_t closure_gap_ns = 0;

    // ---- 目标时间轴为什么会动（P13-R）----
    //
    // P13-Q 测出"满载时 `cumD` 可以累积到 −70 ms"，但没说清是谁在动。
    // 正常分支的定义式是 `due = mono + (pts − anchor_pts − elapsed)`，三个自变量
    // 各自都可能动。四个跨度摆在一起，答案就是一道减法：
    //
    //     due_span == arrive_mono_span + pts_span − elapsed_span
    //
    // · `elapsed_span > arrive_mono_span` ⇒ **钟**比墙跑得快（音频位置估计超前）；
    // · `arrive_mono_span < pts_span`     ⇒ **帧到得早**（解码提前交付，arrive 挤在一起）。
    // 两者都会把 `due` 整体往前推，但修的地方完全不同。
    std::int64_t arrive_mono_span_ns = 0;
    std::int64_t elapsed_span_ns = 0;
    std::int64_t due_span_ns = 0;
    std::int64_t pts_span_ns = 0;

    /// **钟在墙上多走了多少**（P14-T）。`= elapsed_span_ns − arrive_mono_span_ns`。
    ///
    /// 正数 = 这只钟比墙钟快：同一段窗口里它自称走过的媒体时间更多。
    ///
    /// 为什么需要这个名字：这一格本来就是上面两个数之差，任何人看一眼都能减出来 ——
    /// 但**没有人减**。P14-S 把 4 份越界登记进表里，状态卡上写着"成因未查"，
    /// 而答案就在同一个 JSON 的相邻两格里：`p13r-4k60-r3` 的 426 703 545 ns
    /// 与"墙钟比实测跨度短 427 ms"是同一件事。差一个名字，就差了整整一张卡。
    ///
    /// 它和 `av_drift` 的关系是本卡最要紧的一句话：
    ///
    /// · `av_drift`（= `lateness`）= **视频相对这只钟**走了偏多少；
    /// · `clock_lead_ns`           = **这只钟相对墙**走了偏多少。
    ///
    /// 前者**不含**后者。钟自称走了 7.967 s、视频就按 7.967 s 排，两者之差永远是 125 µs；
    /// 而真实听起来差的是 427 ms（`113` 卡的真机证据：`av_drift` p50 = 125 µs）。
    /// 于是"`lateness` 等于音画偏差"这句话只在"钟说的是真的"时成立 ——
    /// 而那个前提此前从未被判过。
    ///
    /// **不带 `measured`**：这是两个已出版的数相减，不是测量（与 `undelivered_frames`
    /// 同一条理由）。真正的可测性由 `clock_supplied_frames` 守：它为 0 时四跨度全 0，
    /// 本格也必然 0 —— 那个 0 不可读作"钟很准"。
    std::int64_t clock_lead_ns = 0;

    /// 带上了钟读数的呈现帧数（`due_at_ns != 0`）。
    ///
    /// 小于 `presented` 只有一种原因：**调用方漏传** —— 那四格跨度会全是 0，
    /// 而"全 0"看起来像"这一轮没动"，正好是最容易被读错的那种假观测。
    /// 校验器 R1 拿它当接线自证（`86`/`88` 两张卡各撞到过一次同类漏接）。
    std::int64_t clock_supplied_frames = 0;

    // ---- 参考系的两份答案（P14-U）----
    //
    // `av_drift` 是"视频相对这只钟"，而 `due` 由这只钟算出 —— 所以它**结构性**
    // 看不见"钟本身跑偏"。P14-T 的真机证据把这句话钉死了：
    // `p13r-4k60-r3` 的钟比墙多走了 426.7 ms，同轮 `av_drift.p50 = 125 µs`。
    //
    // 这两格是"另一个参考系"的那一份。它们不回填、不改任何已有字段：
    // 第一份（`lateness` / `av_drift`）仍然是相对钟的，第二份是相对墙的，
    // **并排印出来**才谈得上"音画偏差到底是多少"。

    /// 钟相对墙的逐帧领先量轨迹。含"跳"还是"跑偏"的分辨力（`LeadVerdict`）。
    ///
    /// 与 `clock_lead_ns` 同量不同口径：那个用 arrive 首末两点，这个用 present 逐帧。
    /// 两者之差应在一帧以内（判据 U1），**不合并成一个字段** ——
    /// 一个是"整场多走了多少"，一个是"怎么走过去的"，答的是两个问题。
    LeadTrace lead_trace{};

    /// 视频相对**墙钟**的偏移摘要。`= lateness − lead`。
    ///
    /// 命名见 `pace_wall_offset.hpp`：它不叫 `vs_audio`，因为项目里没有第二个时钟源，
    /// 我们没测到 DAC。它是"用墙钟当耳朵的替身"，替身的可信度在下一格。
    WallOffset wall_offset{};

    /// `wall_offset` 的误差上界（ns）：`max(一帧, kWallReferencePpm × 跨度)`。
    ///
    /// 出版它而不是写进文档，是因为它是一个**假设**（消费级晶振 ±50 ppm），
    /// 不是实测。争这个数的人应该能看见它、改它，并立刻看到上界跟着变。
    std::int64_t wall_reference_bound_ns = 0;

    /// 进了 `schedule_lead` 直方图的样本数。`kCpuPaced` 路径恒为 0。
    std::int64_t scheduled_samples = 0;

    // ---- A/V 偏差（P12-C8）----
    //
    // 这里**刻意没有** `av_drift` 直方图。原因是一次真机测量推翻了最初的写法：
    //
    // `arrive()` 里的 `lateness_ns` 是"这一帧到达时相对到期时刻的偏移"，它只在
    // **CPU 睡到 `due_at`** 的路径上等于音画偏差。`kSurfaceAtTime` 路径不睡，
    // 于是连交 3 帧、`lateness` 恒等于提交提前量（实测 p50 = −99.875 ms = 3 帧，
    // 见 `p12c8-av-r1-sync.json`），跟音画同步没有半点关系。
    //
    // 真正等于音画偏差的量是 `note_presented()` 里的 `present_at − due_at`
    // （推导见 `24-p12c8-design.md` §四之二），也就是已有的 `lateness` 直方图。
    // 再存一份叫 `av_drift` 的会变成同一笔账记两处，早晚不一致。
    // 于是证据层直接从 `lateness` 换算，并由 `present_schedule` 判定能不能算。

    /// 逐帧记录的时钟分档（`AudioClockSourceKind`）。
    ///
    /// 为什么按帧数而不是按查询次数：G7 关心的是"呈现出来的画面有多少帧
    /// 是靠降级时钟对齐的"，不是"时钟内部重试了几次"。
    std::int64_t clock_primary_frames = 0;   ///< 主源（硬件时间戳）
    std::int64_t clock_fallback_frames = 0;  ///< 回退档（frames_read / frames_written）
    std::int64_t clock_system_frames = 0;    ///< 没有音轨，用的是独立单调时钟

    /// 建锚时刻的两侧位置（P12-C8）。被锚点吸收掉的常量在这里，不在 `av_drift` 里。
    std::int64_t anchor_pts_ns = 0;
    std::int64_t anchor_position_ns = 0;

    /// 首个/最后一个"交出去"的单调时刻（C8）。两者相减 = 呈现跨度（墙钟）。
    ///
    /// 有了它才能算播放速率：媒体时间走了多少 / 墙钟走了多少。
    /// 这是本项目唯一一个**不与节拍器结论循环**的检查 —— 它只看墙钟，
    /// 而 `av_drift`、`schedule_lead` 都是拿节拍器自己的目标时刻做基准。
    /// 速率掉到 0.5 或冲到 2.0 时，上面那些指标可能全都"完美"。
    ///
    /// 两条呈现路径共用：`note_presented` 与 `note_scheduled` 都会更新，
    /// 因为两者的时间戳都代表"这一帧按计划该出现了"的节奏。
    std::int64_t first_present_mono_ns = 0;
    std::int64_t last_present_mono_ns = 0;

    /// `|lateness| > frame_duration` 的帧数 —— G6 的"抖动是否超过一帧"直接看它。
    std::int64_t over_budget_frames = 0;

    // ---- 帧时长 ----
    std::int64_t frame_duration_ns = 0;
    double derived_fps = 0.0;

    // ---- 派生 ----
    double drop_ratio = 0.0;

    // ---- 派生量各自量到没有（P14-L）----
    //
    // 上面那几格都是**算出来**的，不是直接观测的。它们的上游没样本时必然是 0，
    // 而 0 在每一格里的含义都是"好消息"：
    //
    //   present_timestamp_measured  为 false ⇒ lateness 一个样本都没有
    //   over_budget_measured        为 false ⇒ `over_budget_frames` 的 0 不是"没有超预算帧"
    //   catch_up_measured           为 false ⇒ `catch_up_events` 的 0 不是"没有追赶"
    //   drop_ratio_measured         为 false ⇒ `drop_ratio` 的 0.0 不是"零丢帧"
    //
    // 真机上的确发生过：`p14k-4k60`（`surface_at_time`）里 `lateness_ns.samples = 0`
    // 而 `over_budget_frames = 0`，G6 的判据就读着这个 0 过去了 ——
    // **G6 在默认排程档上从未被验证过**（见 `136-p14k-status.md` §三.2）。
    //
    // `over_budget_measured` **不是**独立判断出来的，它是 `present_timestamp_measured`
    // 的复制：两条链各判一次，迟早会在某个边界上分家，而分家之后证据会同时说
    // "lateness 没量到"和"超预算帧量到了"。
    MeasuredCounter present_timestamp_measured{};
    MeasuredCounter over_budget_measured{};
    MeasuredCounter catch_up_measured{};
    MeasuredCounter drop_ratio_measured{};
};

/// 聚合器。**非线程安全** —— 只该在决策它的那个线程里用。
class PaceStats {
public:
    void note_waited(std::int64_t wait_ns) noexcept;

    /// 记一次呈现。
    ///
    /// @param lateness_ns `present_at − due_at`
    /// @param media_ns   本帧 pts。抽稀门按它判（P13-D）；传 0 会让序列认为
    ///                    媒体时间不前进，趋势随之 `usable = false` —— 宁可不可用，
    ///                    也不按帧序号悄悄抽稀。
    /// @param step       目标时刻偏差（P13-P）。不给时 `due_step_supplied` 不增，
    ///                    `due_side` / `present_side` 的判定随之失效 —— 默认值是给
    ///                    单测与"这条路径本来就没有 due 语义"的调用方留的，不是给出货路径留的。
    void note_presented(std::int64_t lateness_ns, std::int64_t media_ns,
                        DueStep step = {}) noexcept;

    /// 锚点重建（seek / 换片源 / 停再起）。由 `FramePacer::reset()` 调用。
    ///
    /// 只切漂移序列的段：`lateness` 的零点换了，跨过这一刻的回归拟合的是
    /// 两个零点之间的跳变，不只是速率差。累计计数一律不动。
    void note_anchor_reset() noexcept;
    void note_dropped(std::int64_t lateness_ns) noexcept;
    void note_catch_up_frame(std::int64_t lateness_ns) noexcept;
    void note_interval(std::int64_t deviation_ns) noexcept;
    void note_health(ClockHealth health) noexcept;

    /// 记一次"累积漂移重锚"（P13-S）。每帧最多一次。
    ///
    /// @param correction_ns 这一帧实际修正的幅度（P13-AC 之后有上限）
    void note_drift_rebase(std::int64_t correction_ns) noexcept;

    /// 记一条"提交提前量"（`submit_at - due_at`，负表示提前）。仅 `kSurfaceAtTime` 路径用。
    void note_scheduled(std::int64_t lead_ns) noexcept;

    /// 记一帧的时钟分档。每帧调一次，与是否呈现无关。
    void note_clock_kind(core::audio::AudioClockSourceKind kind) noexcept;

    /// 记一次"交出去"的单调时刻（首帧写入，之后只更新末帧）。
    void note_present_mono(std::int64_t mono_ns) noexcept;

    [[nodiscard]] std::int64_t presented() const noexcept { return presented_; }
    [[nodiscard]] std::int64_t dropped() const noexcept { return dropped_; }

    [[nodiscard]] PaceSummary summary(std::int64_t frame_duration_ns,
                                      ClockHealth health) const noexcept;

private:
    std::int64_t presented_ = 0;
    std::int64_t dropped_ = 0;
    std::int64_t catch_up_frames_ = 0;
    std::int64_t waited_ = 0;
    std::int64_t wait_total_ns_ = 0;
    std::int64_t latency_samples_ = 0;
    std::int64_t extrapolated_frames_ = 0;
    std::int64_t free_run_frames_ = 0;
    SignedHistogram lateness_{};
    SignedHistogram interval_{};
    SignedHistogram dropped_lateness_{};
    SignedHistogram schedule_lead_{};
    std::int64_t scheduled_samples_ = 0;
    SignedHistogram sampled_lateness_{};
    std::int64_t sampled_samples_ = 0;
    DriftSeries drift_{};
    /// 最差帧 + 前后邻帧（P13-P）。P13-F 的 `frame_worst_` 已并入这里：
    /// 两者是同一帧的两面，分开维护等于把同一次比较写两遍。
    WorstEpisodeRecorder episode_{};
    std::int64_t due_step_frames_ = 0;
    std::int64_t due_step_supplied_ = 0;
    bool has_span_ = false;
    std::int64_t first_arrive_mono_ns_ = 0;
    std::int64_t first_elapsed_ns_ = 0;
    std::int64_t first_due_ns_ = 0;
    std::int64_t first_pts_ns_ = 0;
    std::int64_t last_arrive_mono_ns_ = 0;
    std::int64_t last_elapsed_ns_ = 0;
    std::int64_t last_due_ns_ = 0;
    std::int64_t last_pts_ns_ = 0;
    std::int64_t clock_supplied_frames_ = 0;
    /// P14-U：两个参考系的记录器。与 `drift_` 一样只该在呈现线程里写。
    LeadTraceRecorder lead_trace_{};
    WallOffsetRecorder wall_offset_{};
    bool has_lateness_ = false;
    std::int64_t first_lateness_ns_ = 0;
    std::int64_t last_lateness_ns_ = 0;
    std::int64_t sum_present_step_ns_ = 0;
    std::int64_t sum_due_step_ns_ = 0;
    std::int64_t clock_primary_frames_ = 0;
    std::int64_t clock_fallback_frames_ = 0;
    std::int64_t clock_system_frames_ = 0;
    std::int64_t drift_rebases_ = 0;
    std::int64_t drift_max_correction_ns_ = 0;
    std::int64_t drift_corrected_total_ns_ = 0;
    std::int64_t first_present_mono_ns_ = 0;
    std::int64_t last_present_mono_ns_ = 0;
    bool has_present_mono_ = false;
};

}  // namespace kivo::video::core::pace
