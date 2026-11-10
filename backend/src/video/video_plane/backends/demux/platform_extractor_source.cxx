#include "video/video_plane/backends/demux/platform_extractor_source.hpp"

#include "video/video_plane/backends/mediacodec/source/mediaformat_encryption.hpp"

#include <media/NdkMediaFormat.h>

#include <algorithm>
#include <string_view>

namespace kivo::video::mediacodec {
namespace {

/// 初始样本缓冲。4K 高码率关键帧实测在 1–2 MB 量级，4 MB 留足余量。
constexpr std::size_t kInitialSampleCapacity = 4U * 1024U * 1024U;

/// 单包上限。超过它说明文件异常或缓冲增长逻辑出错，必须显式失败而不是继续吞。
constexpr std::size_t kMaxSampleCapacity = 64U * 1024U * 1024U;

// "rotation" 的常量形式 AMEDIAFORMAT_KEY_ROTATION 是 API 28 才导出的，而 minSdk 是 26。
// 键名本身自 API 21 起就稳定，直接用字面量，不因 minSdk 丢掉功能。
constexpr const char* kKeyRotation = "rotation";

[[nodiscard]] std::string read_string(AMediaFormat* format, const char* key) {
    const char* value = nullptr;
    if (!AMediaFormat_getString(format, key, &value) || value == nullptr) {
        return {};
    }
    return std::string(value);
}

[[nodiscard]] std::int32_t read_int32(AMediaFormat* format, const char* key,
                                      std::int32_t fallback) {
    std::int32_t value = 0;
    if (!AMediaFormat_getInt32(format, key, &value)) {
        return fallback;
    }
    return value;
}

[[nodiscard]] std::int64_t read_int64(AMediaFormat* format, const char* key,
                                      std::int64_t fallback) {
    std::int64_t value = 0;
    if (!AMediaFormat_getInt64(format, key, &value)) {
        return fallback;
    }
    return value;
}

}  // namespace

PlatformExtractorSource::~PlatformExtractorSource() {
    release();
}

void PlatformExtractorSource::release() {
    if (format_ != nullptr) {
        AMediaFormat_delete(format_);
        format_ = nullptr;
    }
    if (extractor_ != nullptr) {
        AMediaExtractor_delete(extractor_);
        extractor_ = nullptr;
    }
    info_ = VideoTrackInfo{};
    audio_info_ = AudioTrackInfo{};
    kind_ = TrackKind::kVideo;
    track_index_ = -1;
    encryption_ = TrackEncryption{};
}

ExtractOutcome PlatformExtractorSource::open_fd(int fd, std::int64_t offset, std::int64_t length,
                                            const std::string& label, TrackKind kind) {
    release();

    if (fd < 0) {
        return {ExtractOutcome::Code::kOpenFailed, "invalid file descriptor for " + label};
    }
    if (length <= 0) {
        return {ExtractOutcome::Code::kOpenFailed,
                "empty or unreadable file: " + label + " (length=" + std::to_string(length) + ")"};
    }

    AMediaExtractor* extractor = AMediaExtractor_new();
    if (extractor == nullptr) {
        return {ExtractOutcome::Code::kOpenFailed, "AMediaExtractor_new returned null"};
    }

    const media_status_t status =
        AMediaExtractor_setDataSourceFd(extractor, fd, static_cast<off64_t>(offset),
                                        static_cast<off64_t>(length));
    if (status != AMEDIA_OK) {
        AMediaExtractor_delete(extractor);
        return {ExtractOutcome::Code::kOpenFailed,
                "setDataSourceFd failed for " + label + " (status=" +
                    std::to_string(static_cast<int>(status)) + ")"};
    }

    const std::size_t track_count = AMediaExtractor_getTrackCount(extractor);
    for (std::size_t index = 0; index < track_count; ++index) {
        AMediaFormat* candidate = AMediaExtractor_getTrackFormat(extractor, index);
        if (candidate == nullptr) {
            continue;
        }

        const std::string mime = read_string(candidate, AMEDIAFORMAT_KEY_MIME);
        if (!mime_matches_kind(mime, kind)) {
            AMediaFormat_delete(candidate);
            continue;
        }

        const media_status_t select_status =
            AMediaExtractor_selectTrack(extractor, index);
        if (select_status != AMEDIA_OK) {
            AMediaFormat_delete(candidate);
            extractor_ = extractor;
            release();
            return {ExtractOutcome::Code::kTrackSelectFailed,
                    "selectTrack(" + std::to_string(index) + ") failed (status=" +
                        std::to_string(static_cast<int>(select_status)) + ")"};
        }

        extractor_ = extractor;
        format_ = candidate;
        kind_ = kind;
        track_index_ = static_cast<int>(index);
        sample_buffer_.assign(kInitialSampleCapacity, 0);
        // P12-F3B：加密事实在这里抄下来，**不等 read_next_sample() 的那个标志位**。
        // 实测 K40 上 `AMEDIAEXTRACTOR_SAMPLE_FLAG_ENCRYPTED` 对真 CENC 片源恒为 0，
        // 而轨道格式里 `crypto-mode` 一直在 —— 只看标志位会把加密轨当明文播。
        encryption_ = read_track_encryption(candidate);

        if (kind == TrackKind::kAudio) {
            audio_info_.mime = mime;
            audio_info_.sample_rate = read_int32(candidate, AMEDIAFORMAT_KEY_SAMPLE_RATE, 0);
            audio_info_.channel_count = read_int32(candidate, AMEDIAFORMAT_KEY_CHANNEL_COUNT, 0);
            audio_info_.duration_us = read_int64(candidate, AMEDIAFORMAT_KEY_DURATION, -1);

            // 采样率或声道缺失就无法出声，也无从建立音频时钟。
            // 这里必须失败：带着 0 继续走，后面所有时间换算都是无穷大或零。
            if (audio_info_.sample_rate <= 0 || audio_info_.channel_count <= 0) {
                const std::string detail =
                    "audio track reports invalid format (rate=" +
                    std::to_string(audio_info_.sample_rate) +
                    " channels=" + std::to_string(audio_info_.channel_count) + ")";
                release();
                return {ExtractOutcome::Code::kFormatUnreadable, detail};
            }
            return {};
        }

        info_.mime = mime;
        info_.width = read_int32(candidate, AMEDIAFORMAT_KEY_WIDTH, 0);
        info_.height = read_int32(candidate, AMEDIAFORMAT_KEY_HEIGHT, 0);
        info_.duration_us = read_int64(candidate, AMEDIAFORMAT_KEY_DURATION, -1);
        info_.rotation_degrees = read_int32(candidate, kKeyRotation, 0);

        if (info_.width <= 0 || info_.height <= 0) {
            const std::string detail =
                "video track reports invalid geometry " + std::to_string(info_.width) + "x" +
                std::to_string(info_.height);
            release();
            return {ExtractOutcome::Code::kFormatUnreadable, detail};
        }

        return {};
    }

    AMediaExtractor_delete(extractor);
    // P12-F3：`0 tracks` 与 `有轨但没有我们要的` 是两种**完全不同**的故障，必须说成两句话。
    // 实测：截断到 4 KB 的 MP4 会被解封装器"打开成功"但轨数为 0，
    // 原来的句子是"no video track among 0 tracks" —— 读的人会以为"这文件只有音频"，
    // 而事实是"这文件根本不是一份能用的容器"。
    if (track_count == 0) {
        return {ExtractOutcome::Code::kNoVideoTrack,
                label + ": container reports 0 tracks — the file is likely truncated or corrupt, "
                        "not merely missing a " +
                    std::string(track_kind_name(kind)) + " track"};
    }
    return {kind == TrackKind::kAudio ? ExtractOutcome::Code::kNoAudioTrack
                                      : ExtractOutcome::Code::kNoVideoTrack,
            label + ": no " + std::string(track_kind_name(kind)) + " track among " +
                std::to_string(track_count) + " tracks"};
}

SampleReadStatus PlatformExtractorSource::read_next_sample(SampleView& out) {
    out = SampleView{};
    if (extractor_ == nullptr) {
        return SampleReadStatus::kFailed;
    }

    while (true) {
        const ssize_t size =
            AMediaExtractor_readSampleData(extractor_, sample_buffer_.data(), sample_buffer_.size());
        if (size < 0) {
            return SampleReadStatus::kEndOfStream;
        }

        const auto read_bytes = static_cast<std::size_t>(size);

        // readSampleData 不会推进读指针，因此「缓冲可能不够」时可以放大后重读同一包。
        const bool possibly_truncated = read_bytes == sample_buffer_.size();
        if (possibly_truncated && sample_buffer_.size() < kMaxSampleCapacity) {
            sample_buffer_.resize(std::min(sample_buffer_.size() * 2, kMaxSampleCapacity));
            continue;
        }

        out.data = sample_buffer_.data();
        out.size = read_bytes;
        out.presentation_time_us = AMediaExtractor_getSampleTime(extractor_);

        // 平台标志 → 中立语义。取值差异见 sample_view.hpp 的说明，
        // 这里绝不做「原样透传」，否则 ENCRYPTED(2) 会被解码器当成 CODEC_CONFIG(2)。
        const std::uint32_t flags = AMediaExtractor_getSampleFlags(extractor_);
        out.is_sync = (flags & AMEDIAEXTRACTOR_SAMPLE_FLAG_SYNC) != 0;
        out.is_encrypted = (flags & AMEDIAEXTRACTOR_SAMPLE_FLAG_ENCRYPTED) != 0;

        // advance 失败只意味着这是最后一包：数据已经在本包里，下次调用自然返回 EOS。
        AMediaExtractor_advance(extractor_);
        return SampleReadStatus::kOk;
    }
}

std::int64_t PlatformExtractorSource::seek_to(std::int64_t time_us) {
    if (extractor_ == nullptr) {
        return -1;
    }
    if (AMediaExtractor_seekTo(extractor_, time_us, AMEDIAEXTRACTOR_SEEK_PREVIOUS_SYNC) !=
        AMEDIA_OK) {
        return -1;
    }
    // 252：**平台报 OK 不等于跳到了目标**。
    //
    // 真机实测（Shelter 12.19 GB / 4K 原盘，均走 SMB）：
    //   目标 60 s  → landing 50050 ms   正常
    //   目标 180 s → landing 0 ms       画面回到开头，但 seekTo 返回 AMEDIA_OK
    // 上层（DecodeSession::seek_to）只看"landing < 0 才算失败"，于是把"跳到 0"
    // 当成功，用户看到的是"拖了进度条，画面从头开始"。
    //
    // 落点塌回开头说明 PREVIOUS_SYNC 在这个封装上找不到目标附近的同步点
    // （MKV 索引不完整时就是这样）。先换 CLOSEST_SYNC 再试一次 —— 它对
    // 不完整索引更宽容；仍然塌回开头就返回 -1，让上层明确报"seek 失败"，
    // 而不是静默从头播。
    constexpr std::int64_t kCollapseGuardUs = 2'000'000;  // 2 s
    auto landing = AMediaExtractor_getSampleTime(extractor_);
    if (time_us > kCollapseGuardUs && landing >= 0 && landing < kCollapseGuardUs) {
        if (AMediaExtractor_seekTo(extractor_, time_us, AMEDIAEXTRACTOR_SEEK_CLOSEST_SYNC) !=
            AMEDIA_OK) {
            return -1;
        }
        landing = AMediaExtractor_getSampleTime(extractor_);
        if (landing < 0 || landing < kCollapseGuardUs) {
            return -1;
        }
    }
    if (landing >= 0) {
        return landing;
    }
    // P12-F1：返回**实际落点**，不是请求值。
    //
    // 原来这里 `return time_us;`，于是"落点"字段恒等于目标 —— 一个永远成立的值
    // 比没有值更糟：它让"PREVIOUS_SYNC 是否真的落在目标之前"永远无法被证伪。
    // `seekTo` 只回状态码，落点要自己从当前位置读：seek 之后解封装器停在同步帧上，
    // `getSampleTime` 就是那一帧的 pts，而 `read_next_sample` 会先读它再 advance。
    return -1;  // 走到这里说明落点不可读（>= 0 的分支已在上面 return）
}

}  // namespace kivo::video::mediacodec
