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

int FfmpegDemuxSource::index_seek_stream() const {
    if (context_ == nullptr) {
        return -1;
    }
    // 优先视频轨：MKV 的 cues 挂在它身上（实测 653 条）。
    int best_video = -1;
    int best_entries = -1;
    for (unsigned index = 0; index < static_cast<unsigned>(context_->nb_streams); ++index) {
        AVStream* candidate = context_->streams[index];
        if (candidate->codecpar->codec_type != AVMEDIA_TYPE_VIDEO) {
            continue;
        }
        const int entries = avformat_index_get_entries_count(candidate);
        if (entries > best_entries) {
            best_entries = entries;
            best_video = static_cast<int>(index);
        }
    }
    if (best_video >= 0) {
        return best_video;
    }
    // 没有视频轨：退而求其次，用索引条数最多的那条流。
    int best = -1;
    int best_count = -1;
    for (unsigned index = 0; index < static_cast<unsigned>(context_->nb_streams); ++index) {
        const int entries = avformat_index_get_entries_count(context_->streams[index]);
        if (entries > best_count) {
            best_count = entries;
            best = static_cast<int>(index);
        }
    }
    return best >= 0 ? best : stream_index_;
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
    // 252：**这里仍然按本轨倒回**（曾改成按"定位流"，结果起播阶段丢 80 帧）。
    //
    // 倒回的职责只是"回到开头"，本轨足够；改成按视频轨反而会触发 matroska
    // 去解析 cues（跳到文件尾部读索引，冷缓存下是 5.8 GB 的远端跳读），
    // 把起播那 20 秒喂坏。真机对照：改前 #1 窗丢 0 帧，改后丢 80 帧。
    // 真正的远距离定位问题在 [seek_to]，那里已经改成走视频轨的索引。
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

    // 252：**定位一律走"定位流"（通常是视频轨），不按本轨、也不用默认流。**
    //
    // 真机实测（Shelter 12.19 GB / 2×TrueHD + PGS，跳 600 s）：
    //   audemux=[... rc=0 target=594469000 landed=0 avio=962533]
    // rc=0（seek"成功"）、目标正确，但 **avio 只走到 962 KB** —— demux 停在文件开头，
    // 落点算成 0，于是上层 `clock->rebase(0)` 把音频钟拉回开头，
    // 用户看到的就是"拖了进度条，画面从头开始"。
    //
    // 原因有两条，都已修：
    //  1. `select_stream` 里"探测后倒回开头"用的是**本轨**（音轨）定位。
    //     MKV 的 cues 挂在视频轨上，音轨没有可用索引 ⇒ matroska 退化成回退定位，
    //     把 demux 停在文件开头 —— 后面再怎么 seek 都会落回 0。
    //  2. 这里原来用 `stream_index=-1`（默认流）。默认流由 disposition 决定，
    //     多音轨文件里可能正好是**没有索引的音轨**，同样落回开头。
    // 现在统一走视频轨的索引（653 条 cues）。
    // 252：**近距按本轨、远距按"定位流"**。
    //
    // 远距（本卡要修的）：按本轨（音轨）定位会退化成回退定位，落在文件开头
    //   （真机 avio=962533、落点 0）⇒ 音频钟被 rebase(0) ⇒ 画面从头开始。
    //   远距一律走视频轨的索引（653 条 cues），并用 seek 后第一个包的 pts 当落点。
    //
    // 近距（起播那一次定位到 0）：**仍按本轨**。按视频轨定位会让 matroska 去解析
    //   cues（跳到文件尾部读索引），把起播那 20 秒喂坏 —— 真机对照：
    //   改前 #1 窗丢 0 帧，改后丢 80~125 帧。
    constexpr std::int64_t kFarTargetUs = 2'000'000;  // 2 s
    const bool far_target = time_us > kFarTargetUs;
    int seek_stream = stream_index_;
    if (far_target) {
        seek_stream = index_seek_stream();
    } else {
        // 近距（起播那一次定位到 0）走**默认流**。
        // 按本轨定位在音轨上没有索引 ⇒ matroska 退化成回退定位，会把 demux 落在
        // 一个奇怪的位置，把起播那 20 秒喂坏（真机：#1 窗丢 130 帧）。
        // 默认流走的是视频轨的 cues，落点干净。
        const int fallback = av_find_default_stream_index(context_);
        if (fallback >= 0 && fallback < static_cast<int>(context_->nb_streams)) {
            seek_stream = fallback;
        }
    }
    if (seek_stream < 0 || seek_stream >= static_cast<int>(context_->nb_streams)) {
        return -1;
    }
    AVStream* seek_ref = context_->streams[seek_stream];
    const std::int64_t target = av_rescale_q(time_us, kMicrosPerSecond, seek_ref->time_base);

    last_seek_ = SeekDiagnostics{};
    last_seek_.avio_len = avio_length_;
    last_seek_.target_us = time_us;
    last_seek_.backend = backend_name();
    last_seek_.index_entries = avformat_index_get_entries_count(context_->streams[stream_index_]);

    const int seek_rc = av_seek_frame(context_, seek_stream, target, AVSEEK_FLAG_BACKWARD);
    last_seek_.seek_rc = seek_rc;
    if (seek_rc < 0) {
        return -1;
    }
    avformat_flush(context_);
    have_pending_ = false;

    // 定位之后要"读到本轨的包为止"，这个循环**必须有上界**。
    //
    // 真机实测（Shelter 12.19 GB / TrueHD 音轨）：按音轨索引定位后，落点附近
    // 没有音频包，这个循环就一路往前扫**整个文件** —— 实测 >20 s 不返回，
    // 把整个 native seek 一起拖住。
    constexpr int kMaxScanPackets = 512;
    // 落点 = seek 之后读到的**第一个**包的 pts（即定位到的关键帧），
    // 而不是"本轨第一个包的 pts"：
    //   · 用视频关键帧做落点，正是 mpv 的做法（一次 seek 同时定位两条轨）；
    //   · 音轨包可能没有时间戳，用它算落点会得到 0，把音频钟拉回开头。
    std::int64_t landing = -1;
    auto scan_to_own_stream = [this, &landing, time_us]() -> bool {
        int scanned = 0;
        while (true) {
            if (cancelled_) {
                return false;
            }
            const int status = av_read_frame(context_, packet_);
            if (status < 0) {
                return false;
            }
            if (landing < 0) {
                const std::int64_t pts = packet_time(packet_);
                if (pts != 0 || time_us <= 0) {
                    const AVStream* packet_stream =
                        context_->streams[packet_->stream_index];
                    landing = av_rescale_q(pts, packet_stream->time_base, kMicrosPerSecond);
                }
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
        // 扫不到本轨：换文件级索引再试一次（视频轨有 cues，落点必然在视频关键帧上）。
        if (av_seek_frame(context_, seek_stream, target, AVSEEK_FLAG_BACKWARD) < 0) {
            return -1;
        }
        avformat_flush(context_);
        have_pending_ = false;
        landing = -1;
        if (!scan_to_own_stream()) {
            return -1;
        }
    }

    // 落点塌回开头 = 没跳成。**必须报错，不能把 0 当成功**：
    // 上层会拿落点去 rebase 音频钟，0 会把整条管线拉回起播位置。
    constexpr std::int64_t kCollapseGuardUs = 2'000'000;  // 2 s
    if (far_target && landing >= 0 && landing < kCollapseGuardUs) {
        last_seek_.landed_us = landing;
        last_seek_.avio_pos = context_->pb ? context_->pb->pos : -1;
        return -1;
    }

    have_pending_ = true;
    last_seek_.landed_us = landing;
    last_seek_.avio_pos = context_->pb ? context_->pb->pos : -1;
    return landing;
}

}  // namespace kivo::video::demux
