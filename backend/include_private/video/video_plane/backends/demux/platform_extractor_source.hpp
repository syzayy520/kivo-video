#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include <media/NdkMediaExtractor.h>
#include <media/NdkMediaFormat.h>

#include "video/video_plane/backends/demux/demux_source.hpp"

namespace kivo::video::mediacodec {

/// 平台解封装实现（AMediaExtractor）。
///
/// 这是 P12 以来一直在跑的那条路，不改变任何行为：从
/// `mediacodec/source/mediaextractor_source` 搬出来并实现 `demux::DemuxSource`，
/// 只为让 FFmpeg 实现在不改调用点的前提下并排存在。
class PlatformExtractorSource final : public demux::DemuxSource {
public:
    PlatformExtractorSource() = default;
    ~PlatformExtractorSource() override;

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
    [[nodiscard]] const char* backend_name() const override { return "platform"; }

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
