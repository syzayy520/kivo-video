#pragma once

#include <cstdint>
#include <mutex>
#include <string>

#include <android/native_window.h>
#include <media/NdkMediaCodec.h>
#include <media/NdkMediaFormat.h>

#include "video/video_plane/backends/mediacodec/decode/decode_outcome.hpp"
#include "video/video_plane/backends/mediacodec/decode/decoded_frame.hpp"
#include "video/video_plane/backends/mediacodec/decode/video_color_facts.hpp"
#include "video/video_plane/backends/mediacodec/source/sample_view.hpp"

namespace kivo::video::mediacodec {

/// MediaCodec 同步模式视频解码器。
///
/// 只做三件事：把压缩包塞进去、把解好的帧放出来（渲染到 Surface）、被要求时 flush。
/// 明确不做：读文件、计时、丢帧决策、A/V 同步 —— 这些属于会话层。
///
/// 为什么用同步模式而不是异步回调：同步模式下「送包」与「收帧」的时序完全由我们掌控，
/// 背压天然成立（dequeue 返回 TRY_AGAIN 就是平台在说不）。异步回调会把时序交给
/// 平台线程，做零拷贝和精确丢帧时反而要再搭一层队列来补偿。
class MediaCodecVideoDecoder {
public:
    MediaCodecVideoDecoder() = default;
    ~MediaCodecVideoDecoder();

    MediaCodecVideoDecoder(const MediaCodecVideoDecoder&) = delete;
    MediaCodecVideoDecoder& operator=(const MediaCodecVideoDecoder&) = delete;
    MediaCodecVideoDecoder(MediaCodecVideoDecoder&&) = delete;
    MediaCodecVideoDecoder& operator=(MediaCodecVideoDecoder&&) = delete;

    /// 创建解码器并配置为 Surface 输出。
    ///
    /// @param mime 轨道 mime，用于按类型创建（不按 codec 名，见 10-p12a-evidence.md 铁律 1）
    /// @param format 轨道格式，必须是解封装器给出的那一份
    /// @param window 目标窗口，必须已 acquire 且在整个解码期间有效
    [[nodiscard]] DecodeOutcome configure(const std::string& mime, const AMediaFormat* format,
                                          ANativeWindow* window);

    [[nodiscard]] DecodeOutcome start();

    /// 送一包。TRY_AGAIN 表示输入队列满，调用方应先把已解出的帧收走。
    ///
    /// @param timeout_us 等待输入缓冲的上限。0 表示不等待 —— 会话层灌包时用 0，
    ///        这样一次调用能连续塞满多个输入槽，而不是每包都阻塞一次往返。
    [[nodiscard]] DecodeOutcome submit(const SampleView& sample, std::int64_t timeout_us);

    /// 通知解码器输入结束。之后仍要持续 receive 直到拿到 kEndOfStream。
    [[nodiscard]] DecodeOutcome signal_end_of_stream(std::int64_t presentation_time_us);

    /// 收一张帧，**不释放**。kOk / kEndOfStream 表示 `frame` 有效，调用方必须
    /// 随后调 `release_frame()` 把它交出去或丢掉 —— 不交就等于扣着解码器的缓冲。
    ///
    /// @param timeout_us 平台等待上限；0 表示不等待
    [[nodiscard]] DecodeOutcome receive(DecodedFrame& frame, std::int64_t timeout_us);

    /// 交出（或丢弃）一张帧。
    ///
    /// @param render true = 送 Surface 合成；false = 丢掉不上屏（按节拍丢帧走这条）。
    ///        丢帧也必须 release，否则平台再也拿不回这个缓冲槽。
    [[nodiscard]] DecodeOutcome release_frame(std::uint64_t index, bool render);

    /// 交出一张帧，并要求它在 `present_ns` 时刻显示（P12-C7）。
    ///
    /// @param present_ns 单调时钟纳秒，**与 `pace_now_ns()` 同源**（libc++ 的
    ///        `steady_clock` 即 `CLOCK_MONOTONIC`，与 Java `System.nanoTime()` 一致）。
    ///        传 0 或负值等价于 `release_frame(index, true)`。
    ///
    /// 与 `release_frame(index, true)` 的差别只有一处：后者说"尽快显示"，
    /// 前者说"到点再显示"。**它不改变槽位何时归还** —— 槽位仍在
    /// `releaseOutputBuffer` 时占用，所以提前量必须受 BufferQueue 深度约束。
    [[nodiscard]] DecodeOutcome release_frame_at(std::uint64_t index, std::int64_t present_ns);

    /// 清空在途状态（seek 之后调用）。
    [[nodiscard]] DecodeOutcome flush();

    void stop();

    /// 换输出窗口（P12-D1）。**允许在解码运行中调用**。
    ///
    /// @param window 新的输出窗口；传 nullptr 表示"切到 ByteBuffer 模式"，
    ///        用于 Surface 已经销毁、新 Surface 还没到的空窗期。
    ///
    /// 语义来自平台：`AMediaCodec_setOutputSurface` 换面时，**已经进队列的帧会被丢弃**。
    /// 这不是缺陷 —— 那些帧本来就要显示在一个已经消失的面上。
    ///
    /// 与 `release_frame*()` 之间用一把内部互斥量串起来：NDK 没有为
    /// "换面 vs 交帧"给出并发保证，而这两件事都在碰同一个 `AMediaCodec*`。
    /// 只护这两件，不给 `submit`/`receive` 加锁 —— 它们不碰窗口，
    /// 加锁只是让热路径更慢，不增加任何安全性。
    [[nodiscard]] DecodeOutcome set_output_surface(ANativeWindow* window);

    /// 颜色事实（P12-E1）：源侧 profile + 平台实际给的输出格式。
    ///
    /// 在 `start()` 之后才有输出格式 —— `configure()` 只决定"输出到哪"，
    /// 具体格式要等平台初始化完。拿不到就留 -1，**不猜**。
    [[nodiscard]] const VideoColorFacts& color_facts() const noexcept { return color_; }

    /// 平台实际选中的 codec 名。configure 成功后可用，仅用于日志与证据，
    /// **不得**用于任何逻辑判断。
    [[nodiscard]] std::string codec_name() const;

private:
    DecodeOutcome fail(const std::string& message);
    void release();

    AMediaCodec* codec_ = nullptr;
    bool configured_ = false;
    bool started_ = false;

    /// 见 `set_output_surface()` 的注释：只护"换面"与"交帧"这一对。
    std::mutex surface_mutex_;

    /// P12-E1：源 profile（configure 时从轨道格式抄）+ 输出格式（start 后读）。
    VideoColorFacts color_{};
};

}  // namespace kivo::video::mediacodec
