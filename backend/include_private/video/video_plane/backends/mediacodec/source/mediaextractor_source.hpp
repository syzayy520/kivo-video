#pragma once

#include <cstdint>
#include <memory>
#include <string>

#include <media/NdkMediaFormat.h>

#include "video/video_plane/backends/demux/demux_source.hpp"
#include "video/video_plane/backends/mediacodec/source/audio_track_info.hpp"
#include "video/video_plane/backends/mediacodec/source/encryption_facts.hpp"
#include "video/video_plane/backends/mediacodec/source/extract_outcome.hpp"
#include "video/video_plane/backends/mediacodec/source/media_track_info.hpp"
#include "video/video_plane/backends/mediacodec/source/sample_view.hpp"
#include "video/video_plane/backends/mediacodec/source/track_kind.hpp"

namespace kivo::video::mediacodec {

/// 解封装门面。**保留原类名与原公开 API**，让 DecodeSession / AudioPlayback /
/// 各 probe 一行都不用改；内部按容器选择实现：
///
/// - `matroska` / `webm`（EBML 魔数 `1A 45 DF A3`）→ libavformat。
///   原因：Android 13 的平台解封装器不解析 MKV SeekHead→Cues，7.37 GB 片源
///   线性扫描 45 秒返回 0 轨（`docs/evidence/p240/network-native-playback/fd-semantics-r1.md`）。
/// - 其他容器 → 平台 AMediaExtractor。这条路上平板有 r16c 真机稳态证据，
///   一次全换等于把已验证的结论全部作废。
///
/// 选择在**打开前**完成，不是"先试平台失败了再回退"：K40 上平台失败要
/// 45 秒才返回 0 轨，回退等于每次起播先罚 45 秒。
///
/// 用了哪条路径必须可读：`backend_name()` 写进证据与 summary。
class MediaExtractorSource {
public:
    MediaExtractorSource();
    ~MediaExtractorSource();

    MediaExtractorSource(const MediaExtractorSource&) = delete;
    MediaExtractorSource& operator=(const MediaExtractorSource&) = delete;
    MediaExtractorSource(MediaExtractorSource&&) = delete;
    MediaExtractorSource& operator=(MediaExtractorSource&&) = delete;

    [[nodiscard]] ExtractOutcome open_fd(int fd, std::int64_t offset, std::int64_t length,
                                         const std::string& label,
                                         TrackKind kind = TrackKind::kVideo);

    [[nodiscard]] TrackKind kind() const { return impl_ ? impl_->kind() : TrackKind::kVideo; }
    [[nodiscard]] const VideoTrackInfo& track_info() const;
    [[nodiscard]] const AudioTrackInfo& audio_track_info() const;
    [[nodiscard]] const TrackEncryption& track_encryption() const;
    [[nodiscard]] const AMediaFormat* track_format() const {
        return impl_ ? impl_->track_format() : nullptr;
    }

    [[nodiscard]] SampleReadStatus read_next_sample(SampleView& out);
    [[nodiscard]] std::int64_t seek_to(std::int64_t time_us);

    /// 252：转发底层 demux 的 seek 事实（谁跳的、跳没跳、索引有多少条）。
    [[nodiscard]] demux::DemuxSource::SeekDiagnostics seek_diagnostics() const;

    /// 252：源是否还开着（impl_ 是否还在）。seek 前后对比它，能判断"seek 时源被释放了没有"。
    [[nodiscard]] bool has_source() const { return impl_ != nullptr; }

    /// `"platform"` 或 `"ffmpeg"`；未打开时为 `"none"`。
    [[nodiscard]] const char* backend_name() const {
        return impl_ ? impl_->backend_name() : "none";
    }

private:
    void release();

    std::unique_ptr<demux::DemuxSource> impl_;
    VideoTrackInfo empty_video_{};
    AudioTrackInfo empty_audio_{};
    TrackEncryption empty_encryption_{};
};

}  // namespace kivo::video::mediacodec
