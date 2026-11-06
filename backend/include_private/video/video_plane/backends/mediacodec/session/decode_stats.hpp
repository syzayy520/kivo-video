#pragma once

#include <cstdint>

#include "video/video_plane/core/metrics/alloc_probe.hpp"
#include <string>

#include "video/video_plane/backends/mediacodec/decode/dv_config_facts.hpp"
#include "video/video_plane/backends/mediacodec/decode/hdr_delivery_facts.hpp"
#include "video/video_plane/backends/mediacodec/source/encryption_facts.hpp"
#include "video/video_plane/core/seek/seek_budget.hpp"
#include "video/video_plane/backends/mediacodec/decode/video_color_facts.hpp"
#include "video/video_plane/backends/mediacodec/session/decode_metrics_facts.hpp"
#include "video/video_plane/backends/mediacodec/session/decode_pace_stats.hpp"
#include "video/video_plane/backends/mediacodec/session/decode_stage_cost.hpp"
#include "video/video_plane/backends/mediacodec/session/surface_rebuild_tracker.hpp"

namespace kivo::video::mediacodec {

/// 一次解码会话的度量结果。
///
/// 全部是**事实计数**，不含评价。是否达标由证据层按 G1~G10 判定，
/// 会话层只负责如实记录。
struct DecodeStats {
    /// 进程级分配计数（P15-C / G3）。**由会话在 `snapshot()` 里填**，
    /// 与 `seek` 同一条理由：渲染层不该自己去问探针（它会拿到"此刻"而不是"这一轮"）。
    core::metrics::AllocFacts alloc{};

    // ---- 输入侧 ----
    std::int64_t samples_submitted = 0;
    std::int64_t bytes_submitted = 0;
    std::int64_t samples_rejected = 0;  ///< 被显式拒绝的包（如加密样本）

    // ---- 输出侧 ----
    /// 从解码器取走的缓冲数，含末尾的 EOS 缓冲（不是画面）。
    /// `frames_received == frames_rendered + frames_dropped_late + 1`。
    std::int64_t frames_received = 0;
    /// 已经交回平台的缓冲数。**必须等于 `frames_received`**：
    /// 少一个就是有一个输出槽永远回不来，几十帧后解码器会彻底停住。
    std::int64_t frames_released = 0;
    std::int64_t frames_rendered = 0;
    std::int64_t frames_dropped_late = 0;     ///< 按节拍主动丢弃（不是解码失败）
    /// 因为"没有面可上"而丢弃（P12-D1 换面空窗期）。
    /// 闭合关系：`frames_received == frames_rendered + frames_dropped_late
    ///          + frames_discarded_no_surface + 1`（末尾 EOS 缓冲）。
    std::int64_t frames_discarded_no_surface = 0;
    std::int64_t first_frame_latency_us = -1; ///< 从开始送包到首帧上屏；-1 表示无帧

    /// P16-P1B26：当前媒体位置（µs）。`-1` = 还没起播（与 0 µs 严格区分）。
    /// 评审："打断之后回来从哪继续"需要它；UI 进度条也用它。
    std::int64_t position_us = -1;

    /// P16-P1B32：pause 账。`wall_us` 已扣除累计 pause（它是"播放时长"），
    /// 但**音画账不会自动对齐**：pause 期间音频位置冻结、视频 pts 连续，
    /// 于是 sync 证据的 `av_drift.lead_ns` 会累积出 −pause 的偏移，
    /// verdict 判成 `clock_off`。那不是时钟跑偏，是"这一段没播"。
    ///
    /// 出版这两个数，读证据的人才能把 `clock_off` 打折；
    /// 不给的话，pause 之后的每一轮证据都会带着一个假的失步结论。
    std::int64_t pause_count = 0;
    std::int64_t pause_total_us = 0;
    /// **冷启动**首帧时延（P13-B）。与上面那个不是一回事：
    /// 那个在 seek 之后会被 `begin_run()` 重置并重新计，只反映"最后一段"；
    /// 这个只在第一次 `start()` 时取值，seek 不会覆盖它。
    /// 两者混读会得出"4K 超预算"的假结论 —— P13-A 就是这么误判的。
    /// -1 表示没测到（未出帧或刚开始）。
    std::int64_t cold_start_first_frame_us = -1;

    // ---- 时间 ----
    std::int64_t wall_micros = 0;        ///< 收帧段结束时的墙钟耗时
    std::int64_t media_duration_us = -1; ///< 容器时长；-1 表示未知

    // ---- 派生指标 ----
    double decode_fps = 0.0;             ///< frames_rendered / wall
    double speed_vs_realtime = 0.0;      ///< wall / media_duration 的倒数，越大越快

    // ---- 上下文 ----
    std::string mime;
    std::int32_t width = 0;
    std::int32_t height = 0;
    std::string codec_name; ///< 平台实选，可能为空（API 26/27）

    /// 流水线模式（P12-C6）：`split` / `merged`。取证必须能分辨这两档，
    /// 否则同一份表格里两组数字没法归因。
    std::string pipeline_mode = "unknown";

    /// 是否把帧交给了 Surface。false 表示这一轮是"只解码不上屏"的诊断轮，
    /// 此时 `frames_rendered` 读作"解出并交回的帧数"，不是"上屏帧数"。
    bool present_frames = true;

    /// 呈现排程由谁负责（P12-C7）：`"cpu_paced"` | `"surface_at_time"`。
    std::string present_schedule = "surface_at_time";

    /// 四段平台调用的耗时事实。`submit`/`eos` 由送包段写，`receive`/`release` 由收帧段写。
    StageCost submit_cost{};
    StageCost eos_cost{};
    StageCost receive_cost{};
    StageCost release_cost{};

    /// 节拍度量。`enabled == false` 表示本次没开节拍（P12-B 基线那一轮就是如此）。
    DecodePaceStats pace;

    /// Surface 重建（P12-D1）。`rebinds == 0` 表示这一轮没换过面。
    SurfaceRebuildStats surface_rebuild;

    /// 三把以前没有的尺子（P14-A）：CPU 占比、堆分配次数、输出缓冲归属。
    /// 三个都带 `valid` / `reason` / `hook_installed` —— 这一族最容易把"没测到"印成 0。
    DecodeMetricsFacts metrics;

    /// 颜色事实（P12-E1）：源 profile + 平台实际给的输出格式。
    /// 判定（10bit / 8bit / 不确定）不在这里做 —— 那是纯函数 `assess_color()` 的事，
    /// 渲染时才调用，避免让统计结构依赖判定规则。
    VideoColorFacts color;

    /// 轨道加密事实（P12-F3B）。**判定不在这里** —— 那是 `assess_encryption()`。
    TrackEncryption encryption;

    /// seek 事实（P12-F1）。**判定不在这里做** —— 那是 `assess_seek()` 的事，
    /// 渲染时才调用，避免统计结构依赖判定规则（与 `color` 同一条理由）。
    core::seek::SeekFacts seek;

    /// HDR 交付事实（P12-E2）：分类依据、静态元数据字节数、dataspace 设置结果。
    /// 与 `color` 分开：`color` 说的是"位深协商"，这里说的是"HDR 标记有没有送到显示链路"。
    HdrDeliveryFacts hdr;

    /// DV 交付事实（210）：判定 + 透传动作。非 DV 片源也出版（verdict=not_dolby_vision），
    /// 否则"没印"和"没查"分不开 —— 与 hdr.kind 的成对铁律同源。
    DvFacts dv;

    // ---- 终态 ----
    bool completed = false; ///< true 表示跑到自然结束
    std::string error;      ///< 非空表示失败原因
};

}  // namespace kivo::video::mediacodec
