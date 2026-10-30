#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "video/video_plane/backends/mediacodec/source/audio_track_info.hpp"
#include "video/video_plane/backends/mediacodec/source/encryption_facts.hpp"
#include "video/video_plane/backends/mediacodec/source/extract_outcome.hpp"
#include "video/video_plane/backends/mediacodec/source/media_track_info.hpp"
#include "video/video_plane/backends/mediacodec/source/sample_view.hpp"
#include "video/video_plane/backends/mediacodec/source/track_kind.hpp"

#include <media/NdkMediaExtractor.h>

namespace kivo::video::mediacodec {

/// 媒体文件解封装器。独占持有 AMediaExtractor，禁止拷贝。
///
/// 职责边界：打开文件、按 `TrackKind` 选定一条轨道、按序交出压缩包。
/// 明确不做：解码、渲染、时基换算 —— 那些属于 decode / session 层。
///
/// 一次只选中一条轨道。要同时播视频和音频就开两个实例（各自持有独立的
/// AMediaExtractor）—— 这比在一个实例里维护"当前选中哪条"的状态更不容易出错。
///
/// 线程约束：非线程安全。每条解码链各自独占一个实例，不跨线程共享。
class MediaExtractorSource {
public:
    MediaExtractorSource() = default;
    ~MediaExtractorSource();

    MediaExtractorSource(const MediaExtractorSource&) = delete;
    MediaExtractorSource& operator=(const MediaExtractorSource&) = delete;
    MediaExtractorSource(MediaExtractorSource&&) = delete;
    MediaExtractorSource& operator=(MediaExtractorSource&&) = delete;

    /// 从已打开的文件描述符打开媒体并选定一条轨道。重复调用会先释放上一次的资源。
    ///
    /// 为什么用 fd 而不是路径：`AMediaExtractor_setDataSource` 收纯文件系统路径时
    /// 在本机（Android 13）直接返回 AMEDIA_ERROR_MALFORMED(-10002)，
    /// 文件本身完好（md5 一致、ffprobe 可解）。fd 方式在所有版本上都可靠，
    /// 也天然适配分区存储。
    ///
    /// @param fd 只读 fd，由调用方持有；本类只在 open 期间使用它
    /// @param offset 数据起始偏移，通常为 0
    /// @param length 数据长度，必须是真实文件大小
    /// @param label 仅用于错误信息与日志的显示名，不参与 IO
    /// @param kind 选视频轨还是音频轨。默认视频，保持既有调用点不变
    [[nodiscard]] ExtractOutcome open_fd(int fd, std::int64_t offset, std::int64_t length,
                                        const std::string& label,
                                        TrackKind kind = TrackKind::kVideo);

    /// 打开后有效。本次选的是哪一类轨道。
    [[nodiscard]] TrackKind kind() const { return kind_; }

    /// 选中视频轨时的静态信息；选的是音频轨时保持默认构造值。
    [[nodiscard]] const VideoTrackInfo& track_info() const { return info_; }

    /// 选中音频轨时的静态信息；选的是视频轨时保持默认构造值。
    [[nodiscard]] const AudioTrackInfo& audio_track_info() const { return audio_info_; }

    /// 加密事实（P12-F3B）。open 选定轨道时读一次。
    ///
    /// **为什么在 open 时读**：和 `info_` 同一时刻、同一理由 —— 轨道格式只在 open 期间可靠。
    /// 也是这条卡的核心修正：`read_next_sample()` 里的 `is_encrypted` 在本机恒为 false
    /// （平台不设 `ENCRYPTED` 位），真正的信号在轨道格式的 `crypto-mode` 上。
    [[nodiscard]] const TrackEncryption& track_encryption() const { return encryption_; }

    /// 轨道原始格式，含 csd-0/csd-1。未打开时为 nullptr。
    /// 返回的指针归本对象所有，调用方不得 delete。
    [[nodiscard]] const AMediaFormat* track_format() const { return format_; }

    /// 读下一包。读到的数据由内部缓冲持有，下次调用即失效。
    [[nodiscard]] SampleReadStatus read_next_sample(SampleView& out);

    /// 定位到指定时间（微秒）。返回实际落点，失败返回 -1。
    [[nodiscard]] std::int64_t seek_to(std::int64_t time_us);

private:
    void release();

    AMediaExtractor* extractor_ = nullptr;
    AMediaFormat* format_ = nullptr;
    VideoTrackInfo info_{};
    AudioTrackInfo audio_info_{};
    TrackEncryption encryption_{};
    TrackKind kind_ = TrackKind::kVideo;
    int track_index_ = -1;
    std::vector<std::uint8_t> sample_buffer_;
};

}  // namespace kivo::video::mediacodec
