// ffmpeg_local_playback_sessions.cpp — 211：软解旁路会话的桌面端到端测试。
//
// 桌面上跑真 FFmpeg（KIVO_ENABLE_FFMPEG + FFMPEG_ROOT），拿 tests/real 的
// 合成测试片走完整链路：open → start → 计数 sink 收帧 → stop → 证据。
// 测片缺失时 SKIP（exit 0 并打印 skip）——与既有 local_media 测试同款纪律。
//
// 判据（对齐设计稿 §五）：
//   计数闭合 frames_decoded ≥ 25、frames_rendered == frames_decoded（计数 sink 不失败）、
//   end_reason == completed（5 秒片自然到尾）、backend == "ffmpeg"、
//   证据 JSON 是合法 JSON（渲染层少逗号的教训——直接 parse 验证）。
#include <cstdio>
#include <string>
#include <vector>

#if defined(_WIN32)
#include <fcntl.h>
#include <io.h>
#include <windows.h>
#else
#include <fcntl.h>
#include <unistd.h>
#endif

#include "video/video_plane/runtime/local_media/ffmpeg_local_playback_session.hpp"

namespace {

namespace local_media = kivo::video::video_plane::runtime::local_media;

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

/// 计数 sink：全收不失败，顺手记录帧尺寸序列。
class CountingSink final : public local_media::IFfFrameSink {
public:
    bool render(const kivo::cinema_engine::DecodedFrame& frame, std::string& error) override {
        (void)error;
        ++frames;
        last_width = frame.width;
        last_height = frame.height;
        if (frame.pixel_format != "rgba") {
            non_rgba_seen = true;
        }
        return true;
    }
    [[nodiscard]] std::int64_t rendered_frames() const override { return frames; }
    [[nodiscard]] const char* name() const override { return "counting"; }

    std::int64_t frames = 0;
    int last_width = 0;
    int last_height = 0;
    bool non_rgba_seen = false;
};

}  // namespace

int main(int argc, char** argv) {
#if defined(_WIN32) && defined(_DEBUG)
    // Debug CRT 的断言/中止默认弹 GUI 对话框会卡死自动化——改道 stderr，
    // 让 abort 的真话（哪个表达式、哪一行）留在测试输出里。
    _CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_FILE | _CRTDBG_MODE_DEBUG);
    _CrtSetReportFile(_CRT_ASSERT, _CRTDBG_FILE_STDERR);
    _CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_FILE | _CRTDBG_MODE_DEBUG);
    _CrtSetReportFile(_CRT_ERROR, _CRTDBG_FILE_STDERR);
    // 退出期 abort（exit=3）疑似堆损坏：每次分配/释放都做堆校验，
    // 第一处损坏会带着分配现场打进来（慢，仅本测试用）。
    _CrtSetDbgFlag(_CrtSetDbgFlag(_CRTDBG_REPORT_FLAG) | _CRTDBG_CHECK_ALWAYS_DF);
#endif
    std::printf("== 211 ffmpeg_local_playback_session_tests ==\n");

    // --thread-ffmpeg-use：主线程建 AVFormatContext，工作线程只用一次 read_packet。
    // 会话的精确形状：跨线程使用 FFmpeg 句柄是否就是退出期 abort 的来源。
    if (argc > 1 && std::string(argv[1]) == "--thread-ffmpeg-use") {
#if defined(_WIN32)
        const int fd2 = _open("tests/real/sd211/p211_wmv2_asf.wmv", _O_RDONLY | _O_BINARY);
#else
        const int fd2 = open("tests/real/sd211/p211_wmv2_asf.wmv", O_RDONLY);
#endif
        if (fd2 < 0) {
            std::printf("thread-ffmpeg-use: cannot open clip\n");
            return 2;
        }
        local_media::FfFdSource source2;
        source2.fd = fd2;
        source2.length = -1;
        auto cb2 = local_media::make_ff_fd_callbacks(&source2);
        kivo::cinema_engine::FfmpegInputConfig cfg2;
        cfg2.callbacks = cb2;
        auto handle2 = kivo::cinema_engine::FfmpegAdapter::open_custom_io(cfg2);
        if (handle2.native == nullptr) {
            std::printf("thread-ffmpeg-use: open_custom_io failed\n");
            close(fd2);
            return 2;
        }
        std::atomic<int> packets{0};
        std::thread worker([&] {
            auto rr = kivo::cinema_engine::FfmpegAdapter::read_packet(handle2);
            if (rr.success) {
                packets.store(1);
            }
        });
        worker.join();
        kivo::cinema_engine::FfmpegAdapter::free_format_context(handle2);
        close(fd2);
        std::printf("thread-ffmpeg-use: packets=%d, exiting cleanly\n", packets.load());
        return 0;
    }

    // --thread-only：起一条 std::thread（只做文件回调读 + sleep，不碰 FFmpeg），
    // join 后退出。分辨「std::thread 本身」还是「线程里跑 FFmpeg」。
    if (argc > 1 && std::string(argv[1]) == "--thread-only") {
#if defined(_WIN32)
        const int fd2 = _open("tests/real/sd211/p211_wmv2_asf.wmv", _O_RDONLY | _O_BINARY);
#else
        const int fd2 = open("tests/real/sd211/p211_wmv2_asf.wmv", O_RDONLY);
#endif
        if (fd2 < 0) {
            std::printf("thread-only: cannot open clip\n");
            return 2;
        }
        local_media::FfFdSource source2;
        source2.fd = fd2;
        source2.length = -1;
        auto cb2 = local_media::make_ff_fd_callbacks(&source2);
        std::atomic<bool> thread_running{true};
        std::vector<std::uint8_t> buf(65536);
        std::thread worker([&] {
            while (thread_running.load(std::memory_order_relaxed)) {
                (void)cb2.read(cb2.user_data, buf.data(), static_cast<int>(buf.size()));
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
        });
        std::this_thread::sleep_for(std::chrono::milliseconds(300));
        thread_running.store(false);
        worker.join();
        close(fd2);
        std::printf("thread-only: joined, exiting cleanly\n");
        return 0;
    }

    // --decode-inline：adapter 建上下文 + runtime 解码 30 帧（无线程无 sink）。
    // 二分定位退出期 abort：干净 → std::thread 嫌疑；也炸 → 解码器路径嫌疑。
    if (argc > 1 && std::string(argv[1]) == "--decode-inline") {
#if defined(_WIN32)
        const int fd2 = _open("tests/real/sd211/p211_wmv2_asf.wmv", _O_RDONLY | _O_BINARY);
#else
        const int fd2 = open("tests/real/sd211/p211_wmv2_asf.wmv", O_RDONLY);
#endif
        if (fd2 < 0) {
            std::printf("decode-inline: cannot open clip\n");
            return 2;
        }
        local_media::FfFdSource source2;
        source2.fd = fd2;
        source2.length = -1;
        auto cb2 = local_media::make_ff_fd_callbacks(&source2);
        kivo::cinema_engine::FfmpegInputConfig cfg2;
        cfg2.callbacks = cb2;
        auto handle2 = kivo::cinema_engine::FfmpegAdapter::open_custom_io(cfg2);
        if (handle2.native == nullptr) {
            std::printf("decode-inline: open_custom_io failed\n");
            close(fd2);
            return 2;
        }
        std::vector<int> vids;
        std::vector<int> auds;
        std::vector<int> subs;
        kivo::cinema_engine::FfmpegAdapter::enumerate_streams(handle2, vids, auds, subs);
        std::string codec2;
        kivo::cinema_engine::FfmpegAdapter::get_stream_codec_name(handle2, vids.front(),
                                                                  codec2);
        auto codec_handle2 =
            kivo::cinema_engine::FfmpegAdapter::create_codec_context(handle2, vids.front());
        if (codec_handle2.native == nullptr) {
            std::printf("decode-inline: create_codec_context failed\n");
            close(fd2);
            return 2;
        }
        kivo::cinema_engine::RealSoftwareDecodeRuntime decoder2;
        if (!decoder2.adopt_codec_context(codec_handle2.native, codec2, "p211_inline")) {
            std::printf("decode-inline: adopt failed\n");
            close(fd2);
            return 2;
        }
        int decoded = 0;
        while (decoded < 30) {
            auto rr = kivo::cinema_engine::FfmpegAdapter::read_packet(handle2);
            if (rr.eof || !rr.success) {
                break;
            }
            if (rr.packet.stream_index != vids.front()) {
                continue;
            }
            kivo::cinema_engine::KivoPacket packet;
            packet.stream_index = rr.packet.stream_index;
            packet.pts = rr.packet.pts;
            packet.dts = rr.packet.dts;
            packet.duration = rr.packet.duration;
            packet.is_key_frame = rr.packet.is_key_frame;
            packet.data = rr.packet_data;
            auto result = decoder2.decode_video_rgba(packet, "p211_inline");
            if (result.success && !result.frame.frame_data.empty()) {
                ++decoded;
            }
        }
        decoder2.close();
        kivo::cinema_engine::FfmpegAdapter::free_format_context(handle2);
        close(fd2);
        std::printf("decode-inline: decoded %d frames, exiting cleanly\n", decoded);
        return 0;
    }

    // --trio-only：只走 adapter（fd→demux→读 30 包→释放），无会话无线程无 sink。
    // 退出期 abort 的二分定位：此模式也 exit=3 → 毒在 trio/SDK 层；
    // 干净 → 毒在会话引入的东西（线程/sink/计数）。
    if (argc > 1 && std::string(argv[1]) == "--trio-only") {
#if defined(_WIN32)
        const int fd2 = _open("tests/real/sd211/p211_wmv2_asf.wmv", _O_RDONLY | _O_BINARY);
#else
        const int fd2 = open("tests/real/sd211/p211_wmv2_asf.wmv", O_RDONLY);
#endif
        if (fd2 < 0) {
            std::printf("trio-only: cannot open clip\n");
            return 2;
        }
        local_media::FfFdSource source2;
        source2.fd = fd2;
        source2.length = -1;
        auto cb2 = local_media::make_ff_fd_callbacks(&source2);
        kivo::cinema_engine::FfmpegInputConfig cfg2;
        cfg2.callbacks = cb2;
        auto handle2 = kivo::cinema_engine::FfmpegAdapter::open_custom_io(cfg2);
        if (handle2.native == nullptr) {
            std::printf("trio-only: open_custom_io failed\n");
            close(fd2);
            return 2;
        }
        int count = 0;
        while (count < 30) {
            auto rr = kivo::cinema_engine::FfmpegAdapter::read_packet(handle2);
            if (rr.eof || !rr.success) {
                break;
            }
            ++count;
        }
        kivo::cinema_engine::FfmpegAdapter::free_format_context(handle2);
        close(fd2);
        std::printf("trio-only: read %d packets, exiting cleanly\n", count);
        return 0;
    }

    const char* clip = "tests/real/sd211/p211_wmv2_asf.wmv";
#if defined(_WIN32)
    FILE* probe_file = nullptr;
    fopen_s(&probe_file, clip, "rb");
#else
    FILE* probe_file = std::fopen(clip, "rb");
#endif
    if (probe_file == nullptr) {
        std::printf("skip: test clip missing (%s)\n", clip);
        return 0;
    }
    std::fclose(probe_file);

#if defined(_WIN32)
    const int fd = _open(clip, _O_RDONLY | _O_BINARY);
#else
    const int fd = open(clip, O_RDONLY);
#endif
    CHECK(fd >= 0);
    if (fd < 0) {
        std::printf("%d checks / %d failed\n", g_checks, g_failures);
        return 1;
    }

    CountingSink sink;
    local_media::FfmpegLocalPlaybackSession session;
    local_media::FfmpegLocalPlaybackSession::Config config;
    config.fd = fd;
    config.offset = 0;
    config.length = -1;
    config.label = "p211_wmv2_asf.wmv";

    const auto opened = session.open(config, &sink);
    CHECK(opened.ok());
    if (!opened.ok()) {
        std::printf("  open reason: %s\n", opened.message.c_str());
        close(fd);
        std::printf("%d checks / %d failed\n", g_checks, g_failures);
        return 1;
    }
    CHECK(session.backend_name() == "ffmpeg");

    const auto started = session.start();
    CHECK(started.ok());

    // 5 秒片 + 30fps ≈ 150 帧；给足 20 秒墙钟等自然 EOS。
    for (int waited_ms = 0; waited_ms < 20000; waited_ms += 100) {
        if (!session.is_running()) {
            break;
        }
        std::printf("  waiting... %d ms (frames=%lld)\n", waited_ms,
                    static_cast<long long>(sink.frames));
#if defined(_WIN32)
        Sleep(100);
#else
        usleep(100000);
#endif
    }

    std::string evidence;
    const auto stopped = session.stop(evidence);
    CHECK(stopped.ok());
    CHECK(session.is_running() == false);
    CHECK(sink.frames >= 25);            // 5 秒 × 30fps ≈ 150，打足余量
    CHECK(session.counters().frames_rendered == session.counters().frames_decoded);
    CHECK(!evidence.empty());
    CHECK(evidence.find("\"decoder\": { \"backend\": \"ffmpeg\"") != std::string::npos);
    CHECK(evidence.find("\"end_reason\": \"completed\"") != std::string::npos);
    CHECK(!sink.non_rgba_seen);          // decode_video_rgba 的输出承诺

#if defined(_WIN32)
    _close(fd);
#else
    close(fd);
#endif
    // 测试片不删（tests/real 本就 gitignored，反复可用）。

    std::printf("%d checks / %d failed\n", g_checks, g_failures);
    return g_failures == 0 ? 0 : 1;
}
