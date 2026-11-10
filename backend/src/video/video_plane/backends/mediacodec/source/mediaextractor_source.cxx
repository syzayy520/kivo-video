#include "video/video_plane/backends/mediacodec/source/mediaextractor_source.hpp"

#include "video/video_plane/backends/demux/platform_extractor_source.hpp"

#ifdef KIVO_ENABLE_FFMPEG
#include "video/video_plane/backends/ffmpeg/demux/ffmpeg_demux_source.hpp"
#endif

#include <unistd.h>

namespace kivo::video::mediacodec {
namespace {

/// MKV/WebM 的 EBML 魔数。
///
/// 只嗅探 4 个字节：容器层面的确定性判断，不依赖任何平台能力。
/// 判错的后果是"该走 FFmpeg 的没走"，会立刻在轨道数上暴露，不会静默播错。
constexpr std::uint8_t kEbmlMagic[4] = {0x1A, 0x45, 0xDF, 0xA3};

[[nodiscard]] bool is_matroska(const int fd, const std::int64_t offset) {
    std::uint8_t magic[4] = {};
    const ssize_t got = ::pread(fd, magic, sizeof(magic), static_cast<off_t>(offset));
    return got == static_cast<ssize_t>(sizeof(magic)) && magic[0] == kEbmlMagic[0] &&
           magic[1] == kEbmlMagic[1] && magic[2] == kEbmlMagic[2] && magic[3] == kEbmlMagic[3];
}

}  // namespace

MediaExtractorSource::MediaExtractorSource() = default;

MediaExtractorSource::~MediaExtractorSource() {
    release();
}

void MediaExtractorSource::release() {
    impl_.reset();
}

ExtractOutcome MediaExtractorSource::open_fd(const int fd, const std::int64_t offset,
                                             const std::int64_t length,
                                             const std::string& label, const TrackKind kind) {
    release();

    if (fd < 0) {
        return {ExtractOutcome::Code::kOpenFailed, "invalid file descriptor for " + label};
    }

    if (is_matroska(fd, offset)) {
#ifdef KIVO_ENABLE_FFMPEG
        impl_ = std::make_unique<demux::FfmpegDemuxSource>();
#else
        // 没有 FFmpeg 构建时不能假装能解 MKV：直接给可读原因，
        // 而不是退回平台路径让用户等 45 秒拿到 0 轨。
        return {ExtractOutcome::Code::kOpenFailed,
                label + ": MKV demux requires the FFmpeg build, which is not linked here"};
#endif
    } else {
        impl_ = std::make_unique<PlatformExtractorSource>();
    }

    ExtractOutcome outcome = impl_->open_fd(fd, offset, length, label, kind);
    if (!outcome.ok()) {
        // 不在失败后换实现重试：选择是打开前按容器定下的。回退只会把一次
        // 确定的失败变成两次，并把 45 秒的平台扫描叠加在后面。
        impl_.reset();
        return outcome;
    }
    return outcome;
}

const VideoTrackInfo& MediaExtractorSource::track_info() const {
    return impl_ ? impl_->track_info() : empty_video_;
}

const AudioTrackInfo& MediaExtractorSource::audio_track_info() const {
    return impl_ ? impl_->audio_track_info() : empty_audio_;
}

const TrackEncryption& MediaExtractorSource::track_encryption() const {
    return impl_ ? impl_->track_encryption() : empty_encryption_;
}

SampleReadStatus MediaExtractorSource::read_next_sample(SampleView& out) {
    if (!impl_) {
        out = SampleView{};
        return SampleReadStatus::kFailed;
    }
    return impl_->read_next_sample(out);
}

std::int64_t MediaExtractorSource::seek_to(const std::int64_t time_us) {
    return impl_ ? impl_->seek_to(time_us) : -1;
}

}  // namespace kivo::video::mediacodec
