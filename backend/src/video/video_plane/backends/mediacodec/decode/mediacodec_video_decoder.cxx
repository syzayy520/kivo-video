#include "video/video_plane/backends/mediacodec/decode/mediacodec_video_decoder.hpp"

#include "video/video_plane/backends/mediacodec/decode/mediaformat_color_io.hpp"
#include "video/video_plane/backends/mediacodec/decode/mediacodec_codec_name_resolver.hpp"

#include <cstring>

namespace kivo::video::mediacodec {
namespace {

constexpr std::int64_t kSubmitTimeoutUs = 8000;
constexpr std::int64_t kEosPtsUs = 0;

[[nodiscard]] std::string status_text(media_status_t status) {
    return "status=" + std::to_string(static_cast<int>(status));
}

}  // namespace

MediaCodecVideoDecoder::~MediaCodecVideoDecoder() {
    release();
}

void MediaCodecVideoDecoder::release() {
    if (codec_ != nullptr) {
        if (started_) {
            AMediaCodec_stop(codec_);
        }
        AMediaCodec_delete(codec_);
        codec_ = nullptr;
    }
    configured_ = false;
    started_ = false;
}

DecodeOutcome MediaCodecVideoDecoder::fail(const std::string& message) {
    return {DecodeOutcome::Code::kFailed, message};
}

DecodeOutcome MediaCodecVideoDecoder::configure(const std::string& mime,
                                                const AMediaFormat* format,
                                                ANativeWindow* window) {
    if (codec_ != nullptr) {
        return fail("decoder already configured");
    }
    if (format == nullptr) {
        return fail("format is null");
    }
    if (window == nullptr) {
        return fail("surface window is null");
    }

    // P12-E1：源侧 profile 现在抄下来 —— `start()` 之后轨道格式仍然可用，
    // 但那时再去读它就得依赖解封装器的生命周期；在这里抄一份最省事。
    // `AMEDIAFORMAT_KEY_PROFILE` 是 API 28 才导出的，键名本身稳定，写字面量。
    color_ = VideoColorFacts{};
    color_.mime = mime;
    std::int32_t profile = -1;
    if (AMediaFormat_getInt32(const_cast<AMediaFormat*>(format), "profile", &profile)) {
        color_.source_profile = profile;
    }

    // P12-E2：源侧颜色标记。这里读的是**轨道格式**（容器给的），
    // 与 `start()` 之后读的输出格式是两件事，别混。
    const SourceColorTags tags = read_source_color_tags(format);
    color_.source_color_standard = tags.standard;
    color_.source_color_transfer = tags.transfer;
    color_.source_color_range = tags.range;
    color_.hdr_static_info_bytes = tags.static_info_bytes;

    AMediaCodec* codec = AMediaCodec_createDecoderByType(mime.c_str());
    if (codec == nullptr) {
        return fail("no decoder for mime " + mime);
    }

    // 第三个参数给窗口即「解码输出直送 Surface」：不经过 CPU、不分配像素缓冲。
    //
    // P12-E2：源里有颜色标记时，喂给平台的不再是轨道格式本身，而是**复制再写**的一份。
    // 差别不在数值（源已经带着这些键），而在于"我们明确写了"这件事有据可查：
    // 直接传等于"平台碰巧读到什么就是什么"，缺键时由解码器自己猜默认值
    // （E1 已实测它猜成 BT709/SDR）。源里一个颜色键都没有时不做副本 ——
    // 无事可发却多造一次失败机会，是纯粹的净损失。
    media_status_t status = AMEDIA_ERROR_UNKNOWN;  // 每个分支都会覆盖它
    AMediaFormat* explicit_format = nullptr;
    if (tags.any()) {
        explicit_format = make_configure_format(format, tags);
        if (explicit_format == nullptr) {
            AMediaCodec_delete(codec);
            return fail("failed to build an explicit color format for " + mime);
        }
        status = AMediaCodec_configure(codec, explicit_format, window, nullptr, 0);
        if (status == AMEDIA_OK) {
            color_.configure_format_copied = true;
            color_.delivered_color_standard = tags.standard;
            color_.delivered_color_transfer = tags.transfer;
            color_.delivered_color_range = tags.range;
        } else {
            // 回退到原格式：显式副本是我们自己造的，造错了不该让播放失败。
            // 但**必须留下原因** —— 静默回退等于把"下发失败"藏起来（G9）。
            const media_status_t explicit_status = status;
            status = AMediaCodec_configure(codec, format, window, nullptr, 0);
            color_.configure_fallback_reason =
                "explicit color format configure failed (" + status_text(explicit_status) +
                "); fell back to the track format, so nothing was explicitly delivered";
        }
        AMediaFormat_delete(explicit_format);
    } else {
        status = AMediaCodec_configure(codec, format, window, nullptr, 0);
    }

    if (status != AMEDIA_OK) {
        AMediaCodec_delete(codec);
        return fail("configure failed for " + mime + " (" + status_text(status) + ")");
    }

    codec_ = codec;
    configured_ = true;
    return {};
}

DecodeOutcome MediaCodecVideoDecoder::start() {
    if (codec_ == nullptr || !configured_) {
        return fail("start called before configure");
    }
    const media_status_t status = AMediaCodec_start(codec_);
    if (status != AMEDIA_OK) {
        return fail("AMediaCodec_start failed (" + status_text(status) + ")");
    }
    started_ = true;

    // P12-E1：读平台**实际给的**输出格式。
    //
    // 读不到不是错误 —— 播放照常继续，证据里写 -1。**绝不在这里猜一个值**：
    // 猜成 8bit 会把"看不见"变成"确认是 8bit"，而那正是这张卡要防的事。
    //
    // 键名用字面量：`AMEDIAFORMAT_KEY_COLOR_STANDARD` 等是 API 28 才导出的，
    // 键名本身自 API 24 稳定（minSdk 26 只能这样写）。
    if (AMediaFormat* out = AMediaCodec_getOutputFormat(codec_); out != nullptr) {
        std::int32_t value = -1;
        if (AMediaFormat_getInt32(out, "color-format", &value)) {
            color_.negotiated_format = value;
        }
        if (AMediaFormat_getInt32(out, "color-standard", &value)) {
            color_.color_standard = value;
        }
        if (AMediaFormat_getInt32(out, "color-transfer", &value)) {
            color_.color_transfer = value;
        }
        if (AMediaFormat_getInt32(out, "color-range", &value)) {
            color_.color_range = value;
        }
        AMediaFormat_delete(out);
    }
    return {};
}

DecodeOutcome MediaCodecVideoDecoder::submit(const SampleView& sample,
                                             std::int64_t timeout_us) {
    if (codec_ == nullptr || !started_) {
        return fail("submit called before start");
    }
    if (sample.is_encrypted) {
        // 加密轨道需要 MediaCrypto，当前未实现。显式拒绝，绝不静默丢包（G9）。
        return fail("encrypted sample is not supported yet");
    }

    const ssize_t index = AMediaCodec_dequeueInputBuffer(codec_, timeout_us);
    if (index < 0) {
        if (index == AMEDIACODEC_INFO_TRY_AGAIN_LATER) {
            return {DecodeOutcome::Code::kTryAgain, "input queue full"};
        }
        return fail("dequeueInputBuffer returned " + std::to_string(index));
    }

    std::size_t capacity = 0;
    std::uint8_t* buffer = AMediaCodec_getInputBuffer(codec_, static_cast<std::size_t>(index),
                                                      &capacity);
    if (buffer == nullptr) {
        return fail("getInputBuffer returned null for index " + std::to_string(index));
    }
    if (sample.size > capacity) {
        return fail("sample size " + std::to_string(sample.size) +
                    " exceeds input buffer capacity " + std::to_string(capacity));
    }

    if (sample.size > 0) {
        std::memcpy(buffer, sample.data, sample.size);
    }

    // 关键帧标志只用于诊断：解码器自己会解析 NAL 类型。不传 CODEC_CONFIG ——
    // 参数集已经在 format 的 csd-0 里，再传一次会被当成冗余配置包。
    const media_status_t status = AMediaCodec_queueInputBuffer(
        codec_, static_cast<std::size_t>(index), 0, sample.size,
        static_cast<std::uint64_t>(sample.presentation_time_us), 0);
    if (status != AMEDIA_OK) {
        return fail("queueInputBuffer failed (" + status_text(status) + ")");
    }
    return {};
}

DecodeOutcome MediaCodecVideoDecoder::signal_end_of_stream(std::int64_t presentation_time_us) {
    if (codec_ == nullptr || !started_) {
        return fail("signalEndOfStream called before start");
    }

    const ssize_t index = AMediaCodec_dequeueInputBuffer(codec_, kSubmitTimeoutUs);
    if (index < 0) {
        if (index == AMEDIACODEC_INFO_TRY_AGAIN_LATER) {
            return {DecodeOutcome::Code::kTryAgain, "input queue full before EOS"};
        }
        return fail("dequeueInputBuffer for EOS returned " + std::to_string(index));
    }

    const media_status_t status = AMediaCodec_queueInputBuffer(
        codec_, static_cast<std::size_t>(index), 0, 0,
        static_cast<std::uint64_t>(presentation_time_us < 0 ? kEosPtsUs
                                                           : presentation_time_us),
        AMEDIACODEC_BUFFER_FLAG_END_OF_STREAM);
    if (status != AMEDIA_OK) {
        return fail("queueing EOS failed (" + status_text(status) + ")");
    }
    return {};
}

DecodeOutcome MediaCodecVideoDecoder::receive(DecodedFrame& frame, std::int64_t timeout_us) {
    if (codec_ == nullptr || !started_) {
        return fail("receive called before start");
    }

    AMediaCodecBufferInfo info{};
    const ssize_t index = AMediaCodec_dequeueOutputBuffer(codec_, &info, timeout_us);

    if (index >= 0) {
        frame.index = static_cast<std::uint64_t>(index);
        frame.presentation_time_us = static_cast<std::int64_t>(info.presentationTimeUs);
        frame.end_of_stream = (info.flags & AMEDIACODEC_BUFFER_FLAG_END_OF_STREAM) != 0;
        if (frame.end_of_stream) {
            return {DecodeOutcome::Code::kEndOfStream, "decoder reached end of stream"};
        }
        return {};
    }

    switch (index) {
        case AMEDIACODEC_INFO_TRY_AGAIN_LATER:
            return {DecodeOutcome::Code::kTryAgain, "no output frame ready"};
        case AMEDIACODEC_INFO_OUTPUT_FORMAT_CHANGED:
        case AMEDIACODEC_INFO_OUTPUT_BUFFERS_CHANGED:
            // 平台内部状态切换，不是错误；调用方下一轮继续收即可。
            return {DecodeOutcome::Code::kTryAgain, "output state changed"};
        default:
            return fail("dequeueOutputBuffer returned " + std::to_string(index));
    }
}

DecodeOutcome MediaCodecVideoDecoder::set_output_surface(ANativeWindow* window) {
    if (codec_ == nullptr || !configured_) {
        return fail("set_output_surface called before configure");
    }
    const std::lock_guard<std::mutex> guard(surface_mutex_);
    const media_status_t status = AMediaCodec_setOutputSurface(codec_, window);
    if (status != AMEDIA_OK) {
        return fail("setOutputSurface failed (" + status_text(status) + ")");
    }
    return {};
}

DecodeOutcome MediaCodecVideoDecoder::release_frame(std::uint64_t index, bool render) {
    if (codec_ == nullptr || !started_) {
        return fail("release_frame called before start");
    }
    // 与 `set_output_surface()` 互斥：两者都在碰同一个 codec 句柄，
    // 而 NDK 不为这一对提供并发保证。无竞争时是一次几十纳秒的 lock/unlock，
    // 相对 releaseOutputBuffer 本身的微秒级开销可以忽略。
    const std::lock_guard<std::mutex> guard(surface_mutex_);
    // render=true：直接把这一帧交到 Surface 合成队列，CPU 不接触像素数据。
    // render=false：丢掉。仍然必须调用 —— 否则这个输出槽永远不回收。
    const media_status_t status =
        AMediaCodec_releaseOutputBuffer(codec_, static_cast<std::size_t>(index), render);
    if (status != AMEDIA_OK) {
        return fail("releaseOutputBuffer failed (" + status_text(status) + ")");
    }
    return {};
}

DecodeOutcome MediaCodecVideoDecoder::release_frame_at(std::uint64_t index,
                                                       std::int64_t present_ns) {
    if (codec_ == nullptr || !started_) {
        return fail("release_frame_at called before start");
    }
    if (present_ns <= 0) {
        // 没有目标时刻就退化成"尽快显示"，不把 0 当成 1970 年去排程。
        // 注意：这里**不能**先加锁再委托 —— `release_frame()` 自己会加锁，
        // 非递归互斥量会直接自锁。锁放在下面那一行之后。
        return release_frame(index, true);
    }
    // 时间戳早于现在等价于"立即显示"（平台会钳到当前时刻），不会报错也不会丢帧；
    // 所以这里不做钳位 —— 钳了反而掩盖"排程已经过期"这件事。
    const std::lock_guard<std::mutex> guard(surface_mutex_);
    const media_status_t status = AMediaCodec_releaseOutputBufferAtTime(
        codec_, static_cast<std::size_t>(index), present_ns);
    if (status != AMEDIA_OK) {
        return fail("releaseOutputBufferAtTime failed (" + status_text(status) + ")");
    }
    return {};
}

DecodeOutcome MediaCodecVideoDecoder::flush() {
    if (codec_ == nullptr) {
        return fail("flush called before configure");
    }
    // flush 之后解码器回到已启动但无在途数据的状态，可直接继续送包。
    const media_status_t status = AMediaCodec_flush(codec_);
    if (status != AMEDIA_OK) {
        return fail("AMediaCodec_flush failed (" + status_text(status) + ")");
    }
    return {};
}

void MediaCodecVideoDecoder::stop() {
    if (codec_ == nullptr || !started_) {
        return;
    }
    AMediaCodec_stop(codec_);
    started_ = false;
}

std::string MediaCodecVideoDecoder::codec_name() const {
    return resolve_codec_name(codec_);
}

}  // namespace kivo::video::mediacodec
