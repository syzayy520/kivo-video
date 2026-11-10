#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include <media/NdkMediaFormat.h>

extern "C" {
#include <libavcodec/bsf.h>
#include <libavformat/avformat.h>
}

#include "video/video_plane/backends/demux/demux_source.hpp"

namespace kivo::video::demux {

class AvioBridge;

/// 用 libavformat 解封装。
///
/// 存在的唯一理由：Android 13 的系统 MediaExtractor 不解析 MKV 的
/// SeekHead→Cues，对 7.37 GB 片源线性扫描 45 秒后返回 0 轨（证据见
/// `docs/evidence/p240/network-native-playback/fd-semantics-r1.md`）。
/// libavformat 的 matroska 解封装器自己读索引，不依赖平台版本。
///
/// 只做解封装：不解码、不重采样、不做时基之外的任何换算。
/// 读取统一走 `AvioBridge`，最终落到同一个 `HttpRangeFile` 代理 fd，
/// 因此 Range 校验、共享缓存、取消语义与平台路径完全一致。
///
/// 线程约束：非线程安全，每条解码链独占一个实例。音视频同时播就开两个。
class FfmpegDemuxSource final : public DemuxSource {
public:
    FfmpegDemuxSource() = default;
    ~FfmpegDemuxSource() override;

    FfmpegDemuxSource(const FfmpegDemuxSource&) = delete;
    FfmpegDemuxSource& operator=(const FfmpegDemuxSource&) = delete;

    [[nodiscard]] ExtractOutcome open_fd(int fd, std::int64_t offset, std::int64_t length,
                                         const std::string& label,
                                         TrackKind kind) override;

    [[nodiscard]] TrackKind kind() const override { return kind_; }
    [[nodiscard]] const VideoTrackInfo& track_info() const override { return info_; }
    [[nodiscard]] const AudioTrackInfo& audio_track_info() const override { return audio_info_; }
    [[nodiscard]] const TrackEncryption& track_encryption() const override { return encryption_; }
    [[nodiscard]] const AMediaFormat* track_format() const override { return format_; }

    [[nodiscard]] SampleReadStatus read_next_sample(SampleView& out) override;
    [[nodiscard]] std::int64_t seek_to(std::int64_t time_us) override;

    [[nodiscard]] SeekDiagnostics seek_diagnostics() const override { return last_seek_; }
    /// 返回带实测数字的标签，例如 `ffmpeg tb=1/1000 start=14083000 first=14083000`。
    /// 把这些数写进 summary 而不是日志：本机 logcat 读不出任何缓冲（实测 0 行），
    /// 日志等于不存在的证据。
    [[nodiscard]] const char* backend_name() const override { return backend_label_.c_str(); }

    /// 由 JNI 层在取消时置位；AVIO 读取与 find_stream_info 都会检查。
    void set_cancel_flag(const bool* flag);

private:
    void release();
    [[nodiscard]] ExtractOutcome select_stream(TrackKind kind);
    [[nodiscard]] SampleReadStatus deliver_pending(AVPacket* packet, SampleView& out);
    /// 读一包属于目标流的原始包到 `packet_`。跳过其他流、处理取消与 EOF。
    [[nodiscard]] SampleReadStatus read_raw_packet();

    static int interrupt(void* opaque);

    AVFormatContext* context_ = nullptr;
    AvioBridge* bridge_ = nullptr;
    AVPacket* packet_ = nullptr;
    /// HEVC 的 mp4toannexb 过滤器：csd 与样本必须是同一种封装，
    /// 否则解码器能 configure 却几乎不出帧（914 包进、2 帧出）。
    AVBSFContext* bsf_ = nullptr;
    AVPacket* filtered_ = nullptr;
    AMediaFormat* format_ = nullptr;
    VideoTrackInfo info_{};
    AudioTrackInfo audio_info_{};
    TrackEncryption encryption_{};
    TrackKind kind_ = TrackKind::kVideo;
    int stream_index_ = -1;

    /// 252：最近一次 seek 的事实，用于在 summary 里回答"到底跳没跳"。
    SeekDiagnostics last_seek_{};

    /// 252：交给 avio 的文件长度。若它小于真实大小，SEEK_END 就够不到尾部 Cues，
    /// FFmpeg 建不出索引 → 远距离 seek 只能落到 0。
    std::int64_t avio_length_ = -1;
    std::vector<std::uint8_t> sample_buffer_;
    bool have_pending_ = false;
    bool cancelled_ = false;
    /// 打开后填成 `ffmpeg tb=<num>/<den> start=<us> first=<us>`。
    /// 这几个数决定"首帧为什么在 14 秒"，必须能被外部读到，不能只写在注释里。
    std::string backend_label_ = "ffmpeg";
};

}  // namespace kivo::video::demux
