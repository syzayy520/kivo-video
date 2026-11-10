#include "video/video_plane/backends/ffmpeg/demux/ffmpeg_demux_source.hpp"

#include "video/video_plane/backends/ffmpeg/demux/avio_bridge.hpp"
#include "video/video_plane/backends/ffmpeg/demux/codec_config_builder.hpp"

extern "C" {
#include <libavcodec/bsf.h>
#include <libavutil/mathematics.h>
}

#include <algorithm>

namespace kivo::video::demux {
namespace {

/// 单包上限，与平台路径一致。超过它说明容器异常或长度字段损坏，
/// 必须显式失败，不能继续吞内存。
constexpr std::size_t kMaxSampleBytes = 64U * 1024U * 1024U;

constexpr AVRational kMicrosPerSecond{1, 1'000'000};

/// 包的时间戳。**没有 pts 时用 dts，不要直接给 0。**
///
/// 直接给 0 的后果不是"差一点"：所有缺 pts 的包都会变成同一时刻，
/// pacer 看到重复时间戳只会决策极少数几帧，表现是"914 个包进去、2 帧出来"。
/// 平台 extractor 保证包必有时间戳，libavformat 不保证（B 帧重排后
/// 个别包 `pts` 为 `AV_NOPTS_VALUE` 而 `dts` 有效）。
[[nodiscard]] std::int64_t packet_time(const AVPacket* packet) {
    if (packet->pts != AV_NOPTS_VALUE) {
        return packet->pts;
    }
    if (packet->dts != AV_NOPTS_VALUE) {
        return packet->dts;
    }
    return 0;
}

}  // namespace

FfmpegDemuxSource::~FfmpegDemuxSource() {
    release();
}

void FfmpegDemuxSource::release() {
    if (filtered_ != nullptr) {
        av_packet_free(&filtered_);
    }
    if (bsf_ != nullptr) {
        av_bsf_free(&bsf_);
    }
    if (packet_ != nullptr) {
        av_packet_free(&packet_);
    }
    if (format_ != nullptr) {
        AMediaFormat_delete(format_);
        format_ = nullptr;
    }
    if (context_ != nullptr) {
        // CUSTOM_IO 模式下 FFmpeg 不会关闭或释放我们提供的 pb。
        avformat_close_input(&context_);
    }
    delete bridge_;
    bridge_ = nullptr;
    info_ = VideoTrackInfo{};
    audio_info_ = AudioTrackInfo{};
    encryption_ = TrackEncryption{};
    kind_ = TrackKind::kVideo;
    stream_index_ = -1;
    have_pending_ = false;
    sample_buffer_.clear();
}

void FfmpegDemuxSource::set_cancel_flag(const bool* flag) {
    cancelled_ = (flag != nullptr && *flag);
    if (bridge_ != nullptr) {
        bridge_->set_cancelled(flag);
    }
}

int FfmpegDemuxSource::interrupt(void* opaque) {
    auto* self = static_cast<FfmpegDemuxSource*>(opaque);
    return self->cancelled_ ? 1 : 0;
}

ExtractOutcome FfmpegDemuxSource::select_stream(const TrackKind kind) {
    const AVMediaType wanted =
        (kind == TrackKind::kAudio) ? AVMEDIA_TYPE_AUDIO : AVMEDIA_TYPE_VIDEO;

    int first_match = -1;
    for (unsigned index = 0; index < context_->nb_streams; ++index) {
        if (context_->streams[index]->codecpar->codec_type == wanted) {
            first_match = static_cast<int>(index);
            break;
        }
    }
    if (first_match < 0) {
        return {kind == TrackKind::kAudio ? ExtractOutcome::Code::kNoAudioTrack
                                          : ExtractOutcome::Code::kNoVideoTrack,
                std::string("no ") + std::string(mediacodec::track_kind_name(kind)) + " track among " +
                    std::to_string(context_->nb_streams) + " tracks"};
    }

    AVStream* stream = context_->streams[first_match];
    BuiltTrackFormat built = CodecConfigBuilder::build(context_, stream, kind);
    if (!built.ok) {
        return {ExtractOutcome::Code::kFormatUnreadable, built.message};
    }

    stream_index_ = first_match;
    kind_ = kind;
    format_ = built.format;
    info_ = built.video;
    audio_info_ = built.audio;

    // `avformat_find_stream_info` 为了探测轨道会真实消费开头的包，读完后
    // demuxer 停在探测结束处而不是起点。不回到开头，第一次 read 就从那里继续 ——
    // K40 实测表现为「seek 报落点 0、首帧 pts = 14.083 s」，10 秒里只出 2 帧。
    // 平台 extractor 自带这个复位，FFmpeg 必须显式做。
    AVStream* selected = context_->streams[first_match];
    const std::int64_t start =
        (selected->start_time == AV_NOPTS_VALUE) ? 0 : selected->start_time;
    if (av_seek_frame(context_, first_match, start, AVSEEK_FLAG_BACKWARD) < 0) {
        return {ExtractOutcome::Code::kOpenFailed,
                "rewind to start after stream probe failed"};
    }
    avformat_flush(context_);
    have_pending_ = false;

    // 252：把"文件长度"与"索引条数"挂进标签 —— 这两个数字直接决定远距离 seek 能不能成：
    // 长度被截断 → FFmpeg 读不到尾部 Cues → 索引为 0 → seek 只能落到 0。
    backend_label_ = "ffmpeg tb=" + std::to_string(selected->time_base.num) + "/" +
                     std::to_string(selected->time_base.den) + " start=" +
                     std::to_string(start) + " sb=" +
                     std::to_string(static_cast<long long>(selected->start_time)) +
                     " len=" + std::to_string(static_cast<long long>(avio_length_)) +
                     " idx=" + std::to_string(avformat_index_get_entries_count(selected)) +
                     " sidx=" + std::to_string(context_->nb_streams);

    // csd 与样本必须同一种封装。MKV 的 `CodecPrivate` 是 hvcC/avcC（AVCC 形状），
    // 而 MediaCodec 的输入需要 AnnexB（起始码）。两者混用会让解码器
    // configure 成功却几乎不出帧——HEVC 实测 914 包提交只输出 2 帧；
    // H.264 的表现更极端：SPS/PPS 之外的包全部被吞，`samples=600 / frames=0`，
    // 20 秒片源 0.5 秒就被"读完"并判 completed（244 卡四组对照）。
    //
    // 两个过滤器对已经是 AnnexB 的包都会原样透传，所以加上它不会让原本正确的情况变糟。
    // 244 之前这里只处理了 HEVC，H.264-in-MKV 因此整条路走不通。
    const char* filter_name = nullptr;
    switch (selected->codecpar->codec_id) {
        case AV_CODEC_ID_HEVC: filter_name = "hevc_mp4toannexb"; break;
        case AV_CODEC_ID_H264: filter_name = "h264_mp4toannexb"; break;
        default: break;
    }
    if (filter_name != nullptr) {
        const AVBitStreamFilter* filter = av_bsf_get_by_name(filter_name);
        if (filter != nullptr && av_bsf_alloc(filter, &bsf_) == 0) {
            bsf_->time_base_in = selected->time_base;
            if (avcodec_parameters_copy(bsf_->par_in, selected->codecpar) == 0 &&
                av_bsf_init(bsf_) == 0) {
                filtered_ = av_packet_alloc();
            } else {
                av_bsf_free(&bsf_);
            }
        }
    }

    // 能走到这里说明 libavformat 已成功解析出轨道。加密容器在 matroska 里由
    // ContentEncodings 声明，FFmpeg 对其支持有限（通常直接报 malformed）。
    // 因此这里如实记为「明确未加密」，而不是留 -1 让上层判 unknown：
    // 本路径没有平台 crypto-mode 可参考，留空等于把「未知」当「有问题」。
    encryption_.mode_present = true;
    encryption_.crypto_mode = 0;

    return {};
}

ExtractOutcome FfmpegDemuxSource::open_fd(int fd, std::int64_t offset, std::int64_t length,
                                          const std::string& label, const TrackKind kind) {
    avio_length_ = length;
    release();

    if (fd < 0) {
        return {ExtractOutcome::Code::kOpenFailed, "invalid file descriptor for " + label};
    }
    if (length <= 0) {
        return {ExtractOutcome::Code::kOpenFailed,
                "empty or unreadable file: " + label + " (length=" + std::to_string(length) + ")"};
    }

    bridge_ = new AvioBridge(fd, offset, length);
    if (!bridge_->ok()) {
        return {ExtractOutcome::Code::kOpenFailed,
                "avio open failed for " + label + ": " + bridge_->error_message()};
    }

    AVFormatContext* context = avformat_alloc_context();
    if (context == nullptr) {
        return {ExtractOutcome::Code::kOpenFailed, "avformat_alloc_context returned null"};
    }
    context->pb = bridge_->context();
    context->flags |= AVFMT_FLAG_CUSTOM_IO;
    context->interrupt_callback.callback = &FfmpegDemuxSource::interrupt;
    context->interrupt_callback.opaque = this;

    AVFormatContext* opened = context;
    const int open_status = avformat_open_input(&opened, "", nullptr, nullptr);
    if (open_status < 0) {
        // 失败时 FFmpeg 已释放 opened；pb 由 AvioBridge 持有，稍后 release 释放。
        context_ = nullptr;
        return {ExtractOutcome::Code::kOpenFailed,
                "avformat_open_input failed for " + label + " (status=" +
                    std::to_string(open_status) + ")"};
    }
    context_ = opened;

    const int stream_status = avformat_find_stream_info(context_, nullptr);
    if (stream_status < 0) {
        return {ExtractOutcome::Code::kFormatUnreadable,
                "avformat_find_stream_info failed for " + label + " (status=" +
                    std::to_string(stream_status) + ")"};
    }

    packet_ = av_packet_alloc();
    if (packet_ == nullptr) {
        return {ExtractOutcome::Code::kOpenFailed, "av_packet_alloc returned null"};
    }

    return select_stream(kind);
}

SampleReadStatus FfmpegDemuxSource::deliver_pending(AVPacket* packet, SampleView& out) {
    if (packet->size <= 0) {
        av_packet_unref(packet);
        return SampleReadStatus::kEndOfStream;
    }
    if (static_cast<std::size_t>(packet->size) > kMaxSampleBytes) {
        av_packet_unref(packet);
        return SampleReadStatus::kFailed;
    }

    sample_buffer_.assign(packet->data, packet->data + packet->size);

    AVStream* stream = context_->streams[stream_index_];
    const std::int64_t pts = packet_time(packet);

    out.data = sample_buffer_.data();
    out.size = sample_buffer_.size();
    out.presentation_time_us = av_rescale_q(pts, stream->time_base, kMicrosPerSecond);
    out.is_sync = (packet->flags & AV_PKT_FLAG_KEY) != 0;
    out.is_encrypted = false;

    av_packet_unref(packet);
    return SampleReadStatus::kOk;
}

SampleReadStatus FfmpegDemuxSource::read_raw_packet() {
    while (true) {
        if (cancelled_) {
            return SampleReadStatus::kFailed;
        }
        const int status = av_read_frame(context_, packet_);
        if (status == AVERROR_EOF) {
            return SampleReadStatus::kEndOfStream;
        }
        if (status < 0) {
            // 短读/网络错误在 AVIO 里已被记为失败，这里不再重试：
            // 把它当 EOF 会让"网断了"看起来像"播完了"。
            return SampleReadStatus::kFailed;
        }
        if (packet_->stream_index == stream_index_) {
            return SampleReadStatus::kOk;
        }
        av_packet_unref(packet_);
    }
}

SampleReadStatus FfmpegDemuxSource::read_next_sample(SampleView& out) {
    out = SampleView{};
    if (context_ == nullptr || stream_index_ < 0 || packet_ == nullptr) {
        return SampleReadStatus::kFailed;
    }

    if (!have_pending_) {
        const SampleReadStatus read = read_raw_packet();
        if (read != SampleReadStatus::kOk) {
            return read;
        }
    }
    have_pending_ = false;

    // 没有 bsf 时直接交付原始包。
    if (bsf_ == nullptr || filtered_ == nullptr) {
        return deliver_pending(packet_, out);
    }

    // 有 bsf：把原始包送进过滤器，取转换后的包交付。
    // receive 返回 EAGAIN 表示"还需要更多输入"，要接着读下一包再送，
    // 不能把这一包丢掉——丢了就是静默少帧。
    while (true) {
        if (av_bsf_send_packet(bsf_, packet_) < 0) {
            return SampleReadStatus::kFailed;
        }
        const int got = av_bsf_receive_packet(bsf_, filtered_);
        if (got == AVERROR(EAGAIN)) {
            const SampleReadStatus read = read_raw_packet();
            if (read != SampleReadStatus::kOk) {
                return read;
            }
            continue;
        }
        if (got < 0) {
            return SampleReadStatus::kEndOfStream;
        }
        break;
    }
    return deliver_pending(filtered_, out);
}

std::int64_t FfmpegDemuxSource::seek_to(std::int64_t time_us) {
    if (context_ == nullptr || stream_index_ < 0 || packet_ == nullptr) {
        return -1;
    }
    AVStream* stream = context_->streams[stream_index_];
    const std::int64_t target = av_rescale_q(time_us, kMicrosPerSecond, stream->time_base);

    // AVSEEK_FLAG_BACKWARD：落到目标之前最近的关键帧，与平台
    // AMEDIAEXTRACTOR_SEEK_PREVIOUS_SYNC 同语义。返回实际落点、不是目标值。
    last_seek_ = SeekDiagnostics{};
    last_seek_.avio_len = avio_length_;
    last_seek_.target_us = time_us;
    last_seek_.backend = backend_name();
    if (stream_index_ >= 0 && stream_index_ < static_cast<int>(context_->nb_streams)) {
        // FFmpeg 7 的索引条数要问公开 API（AVStream 不再直接暴露该字段）。
        last_seek_.index_entries =
            avformat_index_get_entries_count(context_->streams[stream_index_]);
    }
    // 252：**本轨没有索引条目时，不能按本轨 seek**。
    //
    // MKV 的 cues 是文件级的：视频轨有 653 条（实测），但 TrueHD 音轨未必被 FFmpeg
    // 填进自己的 index。`av_seek_frame(ic, audio_stream, ...)` 在这种情况下会退化成
    // 在整个文件里二分/扫描 —— 12.19 GB 的文件上实测卡住 >20 s（用户看到的就是
    // "拖了进度条，画面从头开始"，因为整个 native seek 都被音频拖住了）。
    //
    // 所以：本轨有索引用本轨（快且精确）；没有就退回**默认轨（视频）+ 文件级索引**
    // 定位（`stream_index = -1` 让 FFmpeg 用默认流），落点再由本轨的第一包决定。
    int seek_stream = stream_index_;
    const int own_entries = avformat_index_get_entries_count(context_->streams[stream_index_]);
    if (own_entries <= 0 || kind_ == TrackKind::kAudio) {
        // 音轨一律用**文件级索引**定位（stream_index = -1 → FFmpeg 用默认流，
        // 即视频轨的 cues）。MKV 的 cues 是文件级的：视频轨实测 653 条，而音轨的
        // 索引往往没被填进 `index_entries` —— 这时 `av_seek_frame(ic, audio, ...)`
        // 会退化成在整个文件里二分，实测 >20 s 不返回（252）。
        // 落到视频关键帧后，往前读几百个包就能碰到交织在一起的音频包。
        seek_stream = -1;
    }
    last_seek_.index_entries = own_entries;
    const int seek_rc = av_seek_frame(context_, seek_stream, target, AVSEEK_FLAG_BACKWARD);
    last_seek_.seek_rc = seek_rc;
    if (seek_rc < 0) {
        return -1;
    }
    avformat_flush(context_);
    have_pending_ = false;

    // 252：定位之后要"读到本轨的包为止"，这个循环**必须有上界**。
    //
    // 真机实测（Shelter 12.19 GB / TrueHD 音轨）：按音轨索引定位后，落点附近
    // 没有音频包，这个循环就一路往前扫**整个文件** —— 实测 >20 s 不返回，
    // 把整个 native seek 一起拖住，用户看到"拖了进度条，画面从头开始"。
    //
    // 现在：扫满 kMaxScanPackets 还没见到本轨的包，就换**文件级索引**
    // （stream_index = -1，用视频轨的 cues）再定位一次；还不行就**快速失败**，
    // 宁可让上层报"这条音轨定位不到"，也不要扫十几分钟。
    constexpr int kMaxScanPackets = 512;
    auto scan_to_own_stream = [this]() -> bool {
        int scanned = 0;
        while (true) {
            if (cancelled_) {
                return false;
            }
            const int status = av_read_frame(context_, packet_);
            if (status < 0) {
                return false;
            }
            if (packet_->stream_index == stream_index_) {
                return true;
            }
            av_packet_unref(packet_);
            if (++scanned >= kMaxScanPackets) {
                return false;
            }
        }
    };
    if (!scan_to_own_stream()) {
        // 换文件级索引再试一次（视频轨有 cues，落点必然在视频关键帧上）。
        if (av_seek_frame(context_, -1, target, AVSEEK_FLAG_BACKWARD) < 0) {
            return -1;
        }
        avformat_flush(context_);
        have_pending_ = false;
        if (!scan_to_own_stream()) {
            return -1;
        }
    }

    have_pending_ = true;
    const std::int64_t pts = packet_time(packet_);
    const std::int64_t landed = av_rescale_q(pts, stream->time_base, kMicrosPerSecond);
    last_seek_.landed_us = landed;
    last_seek_.avio_pos = context_->pb ? context_->pb->pos : -1;
    return landed;
}

}  // namespace kivo::video::demux
