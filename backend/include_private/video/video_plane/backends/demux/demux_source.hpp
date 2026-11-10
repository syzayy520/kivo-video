#pragma once

#include <cstdint>
#include <string>

#include <media/NdkMediaFormat.h>

#include "video/video_plane/backends/mediacodec/source/audio_track_info.hpp"
#include "video/video_plane/backends/mediacodec/source/encryption_facts.hpp"
#include "video/video_plane/backends/mediacodec/source/extract_outcome.hpp"
#include "video/video_plane/backends/mediacodec/source/media_track_info.hpp"
#include "video/video_plane/backends/mediacodec/source/sample_view.hpp"
#include "video/video_plane/backends/mediacodec/source/track_kind.hpp"

namespace kivo::video::demux {

using mediacodec::AudioTrackInfo;
using mediacodec::ExtractOutcome;
using mediacodec::SampleReadStatus;
using mediacodec::SampleView;
using mediacodec::TrackEncryption;
using mediacodec::TrackKind;
using mediacodec::VideoTrackInfo;

/// 容器解封装接口。
///
/// 存在理由只有一个：K40（Android 13）的系统 MediaExtractor 不解析 MKV 的
/// SeekHead→Cues，退化成沿 cluster 线性扫描，7.37 GB 片源 45 秒返回 0 轨；
/// 而同一 fd 的 fstat/SEEK_END/尾部 pread 全部正常（见
/// `docs/evidence/p240/network-native-playback/fd-semantics-r1.md`）。
/// 这是平台解析策略问题，改 I/O 层无效，只能换解封装实现。
///
/// 边界：本接口只负责"打开容器、选一条轨、交出压缩包、定位"。
/// 解码、渲染、时基换算、时钟全部不在这里，也不许被拖进来。
///
/// 一次只选中一条轨；音视频同时播就开两个实例（与现有架构一致）。
/// 线程约束：非线程安全，每条解码链独占一个实例。
class DemuxSource {
public:
    virtual ~DemuxSource() = default;

    DemuxSource(const DemuxSource&) = delete;
    DemuxSource& operator=(const DemuxSource&) = delete;

    /// 从已打开 fd 打开容器并选定一条轨。重复调用先释放上一次资源。
    [[nodiscard]] virtual ExtractOutcome open_fd(int fd, std::int64_t offset,
                                                 std::int64_t length,
                                                 const std::string& label,
                                                 TrackKind kind) = 0;

    [[nodiscard]] virtual TrackKind kind() const = 0;
    [[nodiscard]] virtual const VideoTrackInfo& track_info() const = 0;
    [[nodiscard]] virtual const AudioTrackInfo& audio_track_info() const = 0;
    [[nodiscard]] virtual const TrackEncryption& track_encryption() const = 0;

    /// 轨道原始格式（含 csd-0/csd-1）。指针归实现所有，调用方不得 delete。
    [[nodiscard]] virtual const AMediaFormat* track_format() const = 0;

    /// 读下一包，数据由实现持有，下次调用即失效。
    [[nodiscard]] virtual SampleReadStatus read_next_sample(SampleView& out) = 0;

    /// 定位到指定微秒，返回**实际落点**（关键帧 pts），失败返回 -1。
    [[nodiscard]] virtual std::int64_t seek_to(std::int64_t time_us) = 0;

    /// "platform" 或 "ffmpeg"。必须写进证据与 summary：
    /// 走了哪条解析路径是可复算事实，不能靠猜。
    [[nodiscard]] virtual const char* backend_name() const = 0;

protected:
    DemuxSource() = default;
};

}  // namespace kivo::video::demux
