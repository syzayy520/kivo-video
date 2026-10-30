#pragma once

#include <atomic>
#include <chrono>
#include <cstdint>
#include <string>

#include "video/video_plane/backends/mediacodec/decode/decoded_frame.hpp"
#include "video/video_plane/backends/mediacodec/session/decode_stage_step.hpp"
#include "video/video_plane/backends/mediacodec/session/present_schedule.hpp"

namespace kivo::video::core::pace {
class FramePacer;
}  // namespace kivo::video::core::pace

namespace kivo::video::mediacodec {

class MediaCodecVideoDecoder;
class DecodePipelineGate;
class DecodeFailureSlot;
class DecodePaceSampler;
class SurfaceRebuildTracker;
class StageCostMeter;
struct DecodePipelineCounters;

/// 收帧段：`receive` → 节拍判定（可能睡到 `due_at`）→ `release_frame`。
///
/// 它是 `receive` / `release_frame` 的唯一调用者。明确不做：读文件、`submit`、
/// 失败重试策略 —— 那些属于送包段。
///
/// 为什么节拍必须在这里：`FramePacer::arrive()` 会睡到该帧的到期时刻（30fps 时 33ms）。
/// 这一觉睡在**平台调用之间**，送包段照常灌包，解码器在等待期间不停工 ——
/// 这就是 P12-C6 全部的收益来源。
class DecodeReceiveWorker {
public:
    struct Deps {
        MediaCodecVideoDecoder* decoder = nullptr;
        DecodePipelineGate* gate = nullptr;
        DecodePipelineCounters* counters = nullptr;
        DecodeFailureSlot* failure = nullptr;
        StageCostMeter* receive_cost = nullptr;
        StageCostMeter* release_cost = nullptr;

        /// 空表示不开节拍：收到就交，用于全速吞吐取证（merged/split 的 A/B 都是这一档）。
        core::pace::FramePacer* pacer = nullptr;
        DecodePaceSampler* pace_sampler = nullptr;

        /// 换面时刻跟踪（P12-D1）。空指针合法（单测与纯解码路径不需要）。
        SurfaceRebuildTracker* rebuild_tracker = nullptr;

        /// 面是否已摘掉（P12-D1）。置位期间一律 `render=false`：
        /// 只还缓冲，不碰 Surface。空指针等价于"面一直在"。
        const std::atomic<bool>* surface_detached = nullptr;

        /// 是否把帧交给 Surface。false 是诊断模式（只解码不上屏），
        /// 用来把"解码吞吐"与"呈现链路限速"分开量。此时节拍也一并停用 ——
        /// 没有呈现就没有"该什么时候呈现"这个问题。
        bool present_frames = true;

        /// 呈现排程由谁负责（P12-C7）。只在 `pacer != nullptr && present_frames` 时生效。
        PresentSchedule present_schedule = PresentSchedule::kCpuPaced;

        /// 会话起点，用于首帧延迟与总墙钟。
        std::chrono::steady_clock::time_point started_at{};
    };

    explicit DecodeReceiveWorker(Deps deps);

    /// 跑一轮。
    ///
    /// @param allow_wait 没帧时是否在平台上阻塞一下（2ms）。全速吞吐时给 true，
    ///        否则两个线程会在 `TRY_AGAIN` 上互相刷 CPU。
    [[nodiscard]] StageStep pump(bool allow_wait);

    void run();

    /// 落最终墙钟并补采一次节拍快照。可重复调用，只有第一次生效。
    ///
    /// 必须由"收帧段结束"那一方调用（split 模式下是 `run()` 的尾部，
    /// merged 模式下是编排者的循环尾部），否则播放结束到销毁之间的空闲
    /// 会被算进解码耗时，fps 会莫名其妙地掉一截。
    void finish();

private:
    /// 处理一张已经收到的帧：节拍判定 → 释放 → 记账。返回 kDone 表示收到 EOS 帧。
    StageStep handle_frame(const DecodedFrame& frame);

    /// 没收到帧时的兜底：EOS 已发出却长时间不出帧，说明平台卡住或被驱动吞了。
    StageStep check_eos_stall();

    StageStep abort(const std::string& reason);

    Deps deps_;

    /// 最近一次有进展（收到帧）的时刻。EOS 卡死判定以它为基准，
    /// 而不是以 `started_at` —— 慢速吐帧时前者不会误判。
    std::chrono::steady_clock::time_point last_progress_at_{};

    std::int64_t frames_since_sample_ = 0;
};

}  // namespace kivo::video::mediacodec
