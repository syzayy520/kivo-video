#include "kivo/cinema_engine/third_party_adapter/ffmpeg_adapter/ffmpeg_adapter.hpp"

#include "ffmpeg_product_log_filter.hpp"

#ifdef KIVO_ENABLE_FFMPEG
extern "C" {
#include <libavformat/avformat.h>
#include <libavutil/error.h>
#include <libavutil/mem.h>
}
#endif

#include <algorithm>
#include <cstdio>

namespace kivo::cinema_engine {

#ifdef KIVO_ENABLE_FFMPEG
namespace {

struct CustomIoOwner {
    FfmpegInputCallbacks callbacks{};
    AVIOContext* avio{nullptr};
};

void destroy_custom_io(void* context) {
    auto* owner = static_cast<CustomIoOwner*>(context);
    if (owner == nullptr) {
        return;
    }
    if (owner->avio != nullptr) {
        av_freep(&owner->avio->buffer);
        avio_context_free(&owner->avio);
    }
    delete owner;
}

int custom_read_packet(void* opaque, std::uint8_t* buffer, const int buffer_size) {
    auto* owner = static_cast<CustomIoOwner*>(opaque);
    if (owner == nullptr || owner->callbacks.read == nullptr || buffer == nullptr) {
        return AVERROR(EIO);
    }
    const int read =
        owner->callbacks.read(owner->callbacks.user_data, buffer, buffer_size);
    if (read == 0) {
        return AVERROR_EOF;
    }
    return read < 0 ? AVERROR(EIO) : read;
}

std::int64_t custom_seek_packet(void* opaque, const std::int64_t offset, const int whence) {
    auto* owner = static_cast<CustomIoOwner*>(opaque);
    if (owner == nullptr || owner->callbacks.seek == nullptr) {
        return AVERROR(EIO);
    }

    const int normalized = whence & ~AVSEEK_FORCE;
    FfmpegIoSeekOrigin origin = FfmpegIoSeekOrigin::Begin;
    if (normalized == AVSEEK_SIZE) {
        origin = FfmpegIoSeekOrigin::Size;
    } else if (normalized == SEEK_CUR) {
        origin = FfmpegIoSeekOrigin::Current;
    } else if (normalized == SEEK_END) {
        origin = FfmpegIoSeekOrigin::End;
    }

    const std::int64_t result =
        owner->callbacks.seek(owner->callbacks.user_data, offset, origin);
    return result < 0 ? AVERROR(EIO) : result;
}

}  // namespace
#endif

FfmpegFormatHandle FfmpegAdapter::open_custom_io(const FfmpegInputConfig& config) {
#ifdef KIVO_ENABLE_FFMPEG
    ffmpeg_adapter::install_product_log_filter();
    if (config.callbacks.read == nullptr || config.callbacks.seek == nullptr) {
        return {};
    }

    const int buffer_size = std::max(config.buffer_size, 4096);
    auto* owner = new CustomIoOwner{};
    owner->callbacks = config.callbacks;

    auto* buffer = static_cast<std::uint8_t*>(av_malloc(static_cast<std::size_t>(buffer_size)));
    if (buffer == nullptr) {
        delete owner;
        return {};
    }

    owner->avio = avio_alloc_context(
        buffer, buffer_size, 0, owner, custom_read_packet, nullptr, custom_seek_packet);
    if (owner->avio == nullptr) {
        av_free(buffer);
        delete owner;
        return {};
    }
    // **必须标可 seek**：`avio_alloc_context` 默认 `seekable = 0`，
    // 即使传了 seek 回调 FFmpeg 也当纯流处理 —— mkv 于是不能读 Cues 索引，
    // 时间戳退化成不可靠的推断值（真机：VC-1 片源全部帧 pts=0，进度条恒 0）。
    // 回调本身是真的支持随机读（`pread`），所以这里如实声明能力。
    owner->avio->seekable = AVIO_SEEKABLE_NORMAL;

    AVFormatContext* context = avformat_alloc_context();
    if (context == nullptr) {
        destroy_custom_io(owner);
        return {};
    }
    context->pb = owner->avio;
    context->flags |= AVFMT_FLAG_CUSTOM_IO;

    if (avformat_open_input(&context, nullptr, nullptr, nullptr) < 0) {
        if (context != nullptr) {
            context->pb = nullptr;
            avformat_free_context(context);
        }
        destroy_custom_io(owner);
        return {};
    }
    if (avformat_find_stream_info(context, nullptr) < 0) {
        avformat_close_input(&context);
        destroy_custom_io(owner);
        return {};
    }

    return {context, owner, destroy_custom_io};
#else
    (void)config;
    return {};
#endif
}

}  // namespace kivo::cinema_engine
