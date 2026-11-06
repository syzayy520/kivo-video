// ffmpeg_fd_callbacks_tests.cpp — 211：fd 输入回调的单测（桌面，纯逻辑 + 真实 fd）
//
// 测的不是"FFmpeg 能不能读"（那是探针/真机的事），是**回调语义本身**：
// 顺序读推进游标、四种 seek 起点各自正确、length 钳制读、EOF 返回 0、
// offset≠0 的数据段基準、负游标拒绝、坏 fd 拒绝。
// 这层是软解会话的地基：它错了，上面所有证据都错。
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#if defined(_WIN32)
#include <fcntl.h>
#include <io.h>
#include <sys/stat.h>
#else
#include <fcntl.h>
#include <unistd.h>
#endif

#include "video/video_plane/runtime/local_media/ffmpeg_fd_callbacks.hpp"

namespace {

namespace mediacodec_ns = kivo::cinema_engine;
namespace fdns = kivo::video::video_plane::runtime::local_media;

int g_failures = 0;
int g_checks = 0;

void check(bool condition, const char* expr, const char* test, int line) {
    ++g_checks;
    if (!condition) {
        ++g_failures;
        std::printf("  FAIL %s:%d  %s\n", test, line, expr);
    }
}
#define CHECK(expr) check((expr), #expr, __func__, __LINE__)

/// 固定 LCG 造 8KB 已知模式（测试自含，不依赖仓库文件）。
std::vector<std::uint8_t> make_pattern(std::size_t size) {
    std::vector<std::uint8_t> data(size);
    std::uint32_t state = 0x12345678u;
    for (std::size_t i = 0; i < size; ++i) {
        state = state * 1664525u + 1013904223u;
        data[i] = static_cast<std::uint8_t>(state >> 24);
    }
    return data;
}

#if defined(_WIN32)
using FdType = int;
FdType open_file(const char* path) { return _open(path, _O_RDWR | _O_CREAT | _O_BINARY, _S_IREAD | _S_IWRITE); }
void close_file(FdType fd) { _close(fd); }
#else
using FdType = int;
FdType open_file(const char* path) { return open(path, O_RDWR | O_CREAT | O_TRUNC, 0600); }
void close_file(FdType fd) { close(fd); }
#endif

/// 回调包装：断言少一层转换。
int read_cb(mediacodec_ns::FfmpegInputCallbacks& cb, std::uint8_t* buf, int size) {
    return cb.read(cb.user_data, buf, size);
}
std::int64_t seek_cb(mediacodec_ns::FfmpegInputCallbacks& cb, std::int64_t off,
                     mediacodec_ns::FfmpegIoSeekOrigin origin) {
    return cb.seek(cb.user_data, off, origin);
}

}  // namespace

int main() {
    std::printf("== 211 ffmpeg_fd_callbacks_tests ==\n");
    const std::vector<std::uint8_t> pattern = make_pattern(8192);
    const char* path = "kivo_fd_cb_test.tmp";

    const FdType fd = open_file(path);
    CHECK(fd >= 0);
    if (fd < 0) {
        std::printf("%d checks / %d failed\n", g_checks, g_failures);
        return g_failures == 0 ? 0 : 1;
    }
#if defined(_WIN32)
    _write(fd, pattern.data(), static_cast<unsigned>(pattern.size()));
#else
    (void)!write(fd, pattern.data(), pattern.size());
#endif

    // ---- 1. 顺序读 + 游标推进 ----
    fdns::FfFdSource source;
    source.fd = fd;
    source.offset = 0;
    source.length = static_cast<std::int64_t>(pattern.size());
    auto cb = fdns::make_ff_fd_callbacks(&source);
    CHECK(cb.read != nullptr && cb.seek != nullptr && cb.user_data == &source);

    std::vector<std::uint8_t> buf(256, 0);
    int got = read_cb(cb, buf.data(), 100);
    CHECK(got == 100);
    CHECK(std::memcmp(buf.data(), pattern.data(), 100) == 0);
    got = read_cb(cb, buf.data(), 100);
    CHECK(got == 100);
    CHECK(std::memcmp(buf.data(), pattern.data() + 100, 100) == 0);

    // ---- 2. Seek Begin / Current / End / Size ----
    CHECK(seek_cb(cb, 500, mediacodec_ns::FfmpegIoSeekOrigin::Begin) == 500);
    got = read_cb(cb, buf.data(), 50);
    CHECK(got == 50);
    CHECK(std::memcmp(buf.data(), pattern.data() + 500, 50) == 0);
    CHECK(seek_cb(cb, -20, mediacodec_ns::FfmpegIoSeekOrigin::Current) == 530);
    got = read_cb(cb, buf.data(), 10);
    CHECK(std::memcmp(buf.data(), pattern.data() + 530, 10) == 0);
    CHECK(seek_cb(cb, -8, mediacodec_ns::FfmpegIoSeekOrigin::End) ==
          static_cast<std::int64_t>(pattern.size()) - 8);
    got = read_cb(cb, buf.data(), 8);
    CHECK(std::memcmp(buf.data(), pattern.data() + pattern.size() - 8, 8) == 0);
    CHECK(seek_cb(cb, 0, mediacodec_ns::FfmpegIoSeekOrigin::Size) ==
          static_cast<std::int64_t>(pattern.size()));

    // ---- 3. EOF：读到尾返回 0 ----
    got = read_cb(cb, buf.data(), 16);
    CHECK(got == 0);

    // ---- 4. length 钳制：数据段比 fd 短 ----
    fdns::FfFdSource clamped;
    clamped.fd = fd;
    clamped.offset = 0;
    clamped.length = 1000;
    auto cb2 = fdns::make_ff_fd_callbacks(&clamped);
    CHECK(seek_cb(cb2, 990, mediacodec_ns::FfmpegIoSeekOrigin::Begin) == 990);
    got = read_cb(cb2, buf.data(), 100);
    CHECK(got == 10);  // 超出 length 的部分被钳掉
    CHECK(std::memcmp(buf.data(), pattern.data() + 990, 10) == 0);
    got = read_cb(cb2, buf.data(), 16);
    CHECK(got == 0);  // 段内 EOF
    CHECK(seek_cb(cb2, 0, mediacodec_ns::FfmpegIoSeekOrigin::Size) == 1000);

    // ---- 5. offset≠0：fd 中段为数据段基准 ----
    fdns::FfFdSource window;
    window.fd = fd;
    window.offset = 1000;
    window.length = 500;
    auto cb3 = fdns::make_ff_fd_callbacks(&window);
    got = read_cb(cb3, buf.data(), 32);
    CHECK(got == 32);
    CHECK(std::memcmp(buf.data(), pattern.data() + 1000, 32) == 0);
    CHECK(seek_cb(cb3, 0, mediacodec_ns::FfmpegIoSeekOrigin::End) == 500);
    CHECK(seek_cb(cb3, 0, mediacodec_ns::FfmpegIoSeekOrigin::Size) == 500);

    // ---- 6. 错误路径：坏 fd / 负游标 / 未知长度的 End ----
    fdns::FfFdSource bad;
    bad.fd = -1;
    auto cbBad = fdns::make_ff_fd_callbacks(&bad);
    CHECK(read_cb(cbBad, buf.data(), 16) == -1);
    CHECK(seek_cb(cbBad, 0, mediacodec_ns::FfmpegIoSeekOrigin::Begin) == -1);

    fdns::FfFdSource unknown_len;
    unknown_len.fd = fd;
    unknown_len.length = -1;  // 未知长度：Size 现场量，End 拒绝
    auto cbUnknown = fdns::make_ff_fd_callbacks(&unknown_len);
    CHECK(seek_cb(cbUnknown, 0, mediacodec_ns::FfmpegIoSeekOrigin::Size) ==
          static_cast<std::int64_t>(pattern.size()));
    CHECK(seek_cb(cbUnknown, -1, mediacodec_ns::FfmpegIoSeekOrigin::End) == -1);
    CHECK(seek_cb(cbUnknown, -5, mediacodec_ns::FfmpegIoSeekOrigin::Begin) == -1);

    close_file(fd);
#if defined(_WIN32)
    _unlink(path);
#else
    (void)!unlink(path);
#endif

    std::printf("%d checks / %d failed\n", g_checks, g_failures);
    return g_failures == 0 ? 0 : 1;
}
