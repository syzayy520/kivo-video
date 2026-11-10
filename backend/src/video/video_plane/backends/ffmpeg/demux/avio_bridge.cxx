#include "video/video_plane/backends/ffmpeg/demux/avio_bridge.hpp"

#include <algorithm>
#include <cerrno>

#include <unistd.h>

extern "C" {
#include <libavutil/error.h>
#include <libavutil/mem.h>
}

namespace kivo::video::demux {
namespace {

/// 128 KiB：一次 `pread` 对应一次代理 fd 读取，而 `HttpRangeFile::onRead`
/// **必须把请求的字节填满才返回**。缓冲越大，单次要凑的字节越多，
/// 网络慢的设备就越容易在 8 秒读超时前凑不齐 —— 平板实测 1 MiB 缓冲直接
/// `SocketException`（`player_reason=网络读取失败（SocketException）`），
/// 而同源同片在 K40 上正常，因为 K40 每次 128 KiB 只要 13–20 ms。
///
/// 128 KiB 与 `HttpRangeFile` 的常规读块一致：够摊薄往返，又不至于
/// 让一次填充跨越多次网络往返。K40 上把 32 KiB 放大到 1 MiB 时
/// `decode_fps` 0.666 → 0.666（无收益），所以缩小回 128 KiB 不会损失吞吐。
constexpr int kIoBufferSize = 128 * 1024;

}  // namespace

AvioBridge::AvioBridge(int fd, std::int64_t offset, std::int64_t length)
    : fd_(fd), offset_(offset), length_(length) {
    if (fd_ < 0) {
        error_message_ = "invalid fd";
        return;
    }
    if (length_ <= 0) {
        error_message_ = "invalid length";
        return;
    }

    // 缓冲必须由 av_malloc 分配：avio_context_free 会一并释放它。
    auto* buffer = static_cast<std::uint8_t*>(av_malloc(kIoBufferSize));
    if (buffer == nullptr) {
        error_message_ = "av_malloc failed";
        return;
    }
    context_ = avio_alloc_context(buffer, kIoBufferSize, 0, this,
                                  &AvioBridge::read_packet, nullptr, &AvioBridge::seek);
    if (context_ == nullptr) {
        av_free(buffer);
        error_message_ = "avio_alloc_context failed";
        return;
    }
    context_->seekable = AVIO_SEEKABLE_NORMAL;
}

AvioBridge::~AvioBridge() {
    if (context_ != nullptr) {
        avio_context_free(&context_);
    }
}

int AvioBridge::read_packet(void* opaque, std::uint8_t* buffer, int buffer_size) {
    auto* self = static_cast<AvioBridge*>(opaque);
    if (self->cancelled_ != nullptr && *self->cancelled_) {
        self->failed_ = true;
        self->error_message_ = "cancelled";
        return AVERROR_EXIT;
    }
    if (self->position_ >= self->length_) {
        return AVERROR_EOF;
    }

    const auto remaining = self->length_ - self->position_;
    const auto want = static_cast<int>(std::min<std::int64_t>(buffer_size, remaining));
    const ssize_t got = ::pread(self->fd_, buffer, static_cast<std::size_t>(want),
                                static_cast<off_t>(self->offset_ + self->position_));
    if (got < 0) {
        self->failed_ = true;
        self->error_message_ = "pread failed errno=" + std::to_string(errno);
        return AVERROR(errno == 0 ? EIO : errno);
    }
    if (got == 0) {
        return AVERROR_EOF;
    }
    self->position_ += got;
    return static_cast<int>(got);
}

std::int64_t AvioBridge::seek(void* opaque, std::int64_t offset, int whence) {
    auto* self = static_cast<AvioBridge*>(opaque);
    if ((whence & AVSEEK_SIZE) != 0) {
        return self->length_;
    }

    std::int64_t target = 0;
    switch (whence & ~AVSEEK_FORCE) {
        case SEEK_SET:
            target = offset;
            break;
        case SEEK_CUR:
            target = self->position_ + offset;
            break;
        case SEEK_END:
            target = self->length_ + offset;
            break;
        default:
            return -1;
    }
    if (target < 0 || target > self->length_) {
        return -1;
    }
    self->position_ = target;
    return target;
}

}  // namespace kivo::video::demux
