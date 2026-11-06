#include "video/video_plane/runtime/local_media/ffmpeg_fd_callbacks.hpp"

#include <cstdio>

#if defined(_WIN32)
#include <io.h>
#else
#include <unistd.h>
#endif

namespace kivo::video::video_plane::runtime::local_media {
namespace {

/// 定位读取一个窗口。Windows 的 `_pread` 偏移是 32 位（long），
/// 媒体文件会越界，所以用 64 位 lseek + read；单一读取者约定见头文件。
std::int64_t positioned_read(int fd, std::int64_t absolute, std::uint8_t* buffer,
                             std::int32_t size) {
#if defined(_WIN32)
    if (_lseeki64(fd, absolute, SEEK_SET) < 0) {
        return -1;
    }
    return static_cast<std::int64_t>(_read(fd, buffer, static_cast<unsigned>(size)));
#else
    return static_cast<std::int64_t>(
        pread(fd, buffer, static_cast<size_t>(size), static_cast<off_t>(absolute)));
#endif
}

std::int64_t fd_length(int fd) {
#if defined(_WIN32)
    const std::int64_t saved = _lseeki64(fd, 0, SEEK_CUR);
    const std::int64_t end = _lseeki64(fd, 0, SEEK_END);
    _lseeki64(fd, saved, SEEK_SET);
    return end;
#else
    const off_t saved = lseek(fd, 0, SEEK_CUR);
    const off_t end = lseek(fd, 0, SEEK_END);
    lseek(fd, saved, SEEK_SET);
    return static_cast<std::int64_t>(end);
#endif
}

int ff_fd_read(void* user_data, std::uint8_t* buffer, int buffer_size) {
    auto* source = static_cast<FfFdSource*>(user_data);
    if (source == nullptr || source->fd < 0 || buffer == nullptr || buffer_size <= 0) {
        return -1;
    }
    const std::int64_t absolute = source->offset + source->cursor;
    if (source->length >= 0 && source->cursor >= source->length) {
        return 0;  // EOF：custom_io 把 0 翻成 AVERROR_EOF
    }
    std::int32_t want = buffer_size;
    if (source->length >= 0) {
        const std::int64_t remain = source->length - source->cursor;
        if (remain < want) {
            want = static_cast<std::int32_t>(remain);
        }
    }
    const std::int64_t got =
        positioned_read(source->fd, absolute, buffer, want);
    if (got <= 0) {
        return got < 0 ? -1 : 0;
    }
    source->cursor += got;
    return static_cast<int>(got);
}

std::int64_t ff_fd_seek(void* user_data, std::int64_t offset,
                        kivo::cinema_engine::FfmpegIoSeekOrigin origin) {
    auto* source = static_cast<FfFdSource*>(user_data);
    if (source == nullptr || source->fd < 0) {
        return -1;
    }
    switch (origin) {
        case kivo::cinema_engine::FfmpegIoSeekOrigin::Begin:
            source->cursor = offset;
            break;
        case kivo::cinema_engine::FfmpegIoSeekOrigin::Current:
            source->cursor += offset;
            break;
        case kivo::cinema_engine::FfmpegIoSeekOrigin::End:
            // End 以数据长度为基准（fd 可能比数据段长），未知长度无法定位。
            if (source->length < 0) {
                return -1;
            }
            source->cursor = source->length + offset;
            break;
        case kivo::cinema_engine::FfmpegIoSeekOrigin::Size:
            // 长度已知时直接回；未知时现场量一次（fd 真实长度兜底）。
            if (source->length >= 0) {
                return source->length;
            }
            return fd_length(source->fd);
    }
    if (source->cursor < 0) {
        return -1;  // AVIO 不允许负位；报错让上层处理而不是钳到 0
    }
    return source->cursor;
}

}  // namespace

kivo::cinema_engine::FfmpegInputCallbacks make_ff_fd_callbacks(FfFdSource* source) {
    kivo::cinema_engine::FfmpegInputCallbacks callbacks;
    callbacks.user_data = source;
    callbacks.read = &ff_fd_read;
    callbacks.seek = &ff_fd_seek;
    return callbacks;
}

}  // namespace kivo::video::video_plane::runtime::local_media
