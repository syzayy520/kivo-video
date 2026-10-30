// color_probe_main.cxx — P12-E1：直接问解码器"能不能吐 10bit"
//
// 为什么需要它：播放器走的是 Surface 输出，`color-format` 报回来的是厂商私有值，
// **8bit 与 10bit 片源给出同一个数**（实测，见 36-p12e1-status.md）。也就是说
// "播放器里查不到位深"这件事是成立的，但"这个解码器支不支持 P010"仍是一个独立问题，
// 而它可以直接问：把 `color-format` 设成 `COLOR_FormatYUVP010` 去 configure 一次。
//
// 三次尝试，缺一不可：
//   A 不设 color-format、不给窗口 → 平台默认给什么
//   B 设 P010、不给窗口           → 能不能协商到 10bit
//   C 设 Surface、不给窗口        → **反证**：它也会失败，说明"失败"不等于"不支持 10bit"
//
// ⚠️ 这个探针**不能**用来证明"播放器输出的是 10bit"。它只说明 codec 在 ByteBuffer
// 模式下接受什么。真正的播放路径是 Surface，两条路不是一回事。
#include <fcntl.h>
#include <unistd.h>

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

#include <media/NdkMediaCodec.h>
#include <media/NdkMediaFormat.h>

#include "video/video_plane/backends/mediacodec/decode/color_format_names.hpp"
#include "video/video_plane/backends/mediacodec/source/mediaextractor_source.hpp"

namespace {

namespace mediacodec = kivo::video::mediacodec;
namespace color_format = kivo::video::mediacodec::color_format;

constexpr const char* kSchema = "kivo.p12e.color-probe/1";

struct Attempt {
    const char* name = "";
    std::string requested;      ///< 请求了什么（"none" / "yuvp010" / "surface"）
    std::string status;         ///< configure 的返回码（可读）
    bool configure_ok = false;
    bool start_ok = false;      ///< configure 之后有没有真的 start 起来
    std::int32_t format_before_start = -1;  ///< start 前读到的（往往是默认值，不可信）
    std::int32_t format_after_start = -1;   ///< start 后读到的（端口协商后的值）
    std::string note;
};

[[nodiscard]] std::string status_name(media_status_t status) {
    switch (status) {
        case AMEDIA_OK:
            return "ok";
        case AMEDIA_ERROR_UNKNOWN:
            return "error_unknown(-10000)";
        case AMEDIA_ERROR_MALFORMED:
            return "error_malformed(-10001)";
        case AMEDIA_ERROR_UNSUPPORTED:
            return "error_unsupported(-10002)";
        case AMEDIA_ERROR_INVALID_OBJECT:
            return "error_invalid_object(-10003)";
        case AMEDIA_ERROR_INVALID_PARAMETER:
            return "error_invalid_parameter(-10004)";
        case AMEDIA_ERROR_INVALID_OPERATION:
            return "error_invalid_operation(-10005)";
        case AMEDIA_ERROR_END_OF_STREAM:
            return "end_of_stream(-10006)";
        default:
            break;
    }
    return "status(" + std::to_string(static_cast<int>(status)) + ")";
}

/// 造一份**独立**的格式：只带 configure 必需的那几项，外加可选 color-format。
///
/// 为什么不用 `AMediaFormat_copy`：那是 API 28 才导出的，而工程 minSdk 26。
/// 逐键抄虽然笨，但没有版本风险，也让人一眼看清"到底喂了哪些键给解码器"。
[[nodiscard]] AMediaFormat* make_format(const AMediaFormat* source, std::int32_t color_format) {
    auto* out = AMediaFormat_new();

    const char* mime = nullptr;
    if (AMediaFormat_getString(const_cast<AMediaFormat*>(source), AMEDIAFORMAT_KEY_MIME, &mime) &&
        mime != nullptr) {
        AMediaFormat_setString(out, AMEDIAFORMAT_KEY_MIME, mime);
    }
    std::int32_t width = 0;
    std::int32_t height = 0;
    if (AMediaFormat_getInt32(const_cast<AMediaFormat*>(source), AMEDIAFORMAT_KEY_WIDTH, &width)) {
        AMediaFormat_setInt32(out, AMEDIAFORMAT_KEY_WIDTH, width);
    }
    if (AMediaFormat_getInt32(const_cast<AMediaFormat*>(source), AMEDIAFORMAT_KEY_HEIGHT, &height)) {
        AMediaFormat_setInt32(out, AMEDIAFORMAT_KEY_HEIGHT, height);
    }

    // csd-0 必须抄：AVC/HEVC 没有 SPS/PPS 连 configure 都过不去，
    // 那样三次尝试全都会失败，"失败"就失去了区分力。
    void* csd = nullptr;
    std::size_t csd_size = 0;
    if (AMediaFormat_getBuffer(const_cast<AMediaFormat*>(source), "csd-0", &csd, &csd_size) &&
        csd != nullptr && csd_size > 0) {
        AMediaFormat_setBuffer(out, "csd-0", csd, csd_size);
    }

    if (color_format >= 0) {
        AMediaFormat_setInt32(out, AMEDIAFORMAT_KEY_COLOR_FORMAT, color_format);
    }
    return out;
}

[[nodiscard]] std::string requested_name(std::int32_t color_format) {
    if (color_format < 0) {
        return "none";
    }
    return color_format::format_name(color_format);
}

/// 读 `getOutputFormat()` 里的 `color-format`；读不到返回 -1。
[[nodiscard]] std::int32_t read_output_format(AMediaCodec* codec) {
    std::int32_t value = -1;
    if (AMediaFormat* out = AMediaCodec_getOutputFormat(codec); out != nullptr) {
        if (!AMediaFormat_getInt32(out, "color-format", &value)) {
            value = -1;
        }
        AMediaFormat_delete(out);
    }
    return value;
}

/// 跑一次 configure（**不给窗口** = ByteBuffer 模式），再 start 一次读协商结果。
[[nodiscard]] Attempt try_configure(const std::string& mime, const AMediaFormat* source,
                                    std::int32_t color_format, const char* name) {
    Attempt attempt;
    attempt.name = name;
    attempt.requested = requested_name(color_format);

    AMediaCodec* codec = AMediaCodec_createDecoderByType(mime.c_str());
    if (codec == nullptr) {
        attempt.status = "createDecoderByType returned null";
        return attempt;
    }

    AMediaFormat* format = make_format(source, color_format);
    const media_status_t status = AMediaCodec_configure(codec, format, nullptr, nullptr, 0);
    attempt.status = status_name(status);
    if (status == AMEDIA_OK) {
        attempt.configure_ok = true;
        attempt.format_before_start = read_output_format(codec);

        // **必须 start 之后再读。** 第一版只读 configure 后的值，结果三次尝试
        // （含"请求 Surface 却不给窗口"这种明显矛盾的）全部 configure 成功、
        // 且都报 nv12 —— 那颗 OMX 组件在 configure 阶段根本不校验 color-format，
        // 于是"configure 成功"什么都不能证明。端口协商发生在 start 之后。
        const media_status_t started = AMediaCodec_start(codec);
        if (started == AMEDIA_OK) {
            attempt.start_ok = true;
            attempt.format_after_start = read_output_format(codec);
            AMediaCodec_stop(codec);
        } else {
            attempt.note = "start failed: " + status_name(started);
        }
    }
    AMediaCodec_delete(codec);
    AMediaFormat_delete(format);
    return attempt;
}

/// 一个格式字段要输出三行（原始值 / 名字 / 位深）。抽出来是为了两个时点写法一致 ——
/// 手写两遍最容易出现"一个带了位深一个没带"这种比的时候才发现的问题。
void print_format_field(const char* key, std::int32_t format, bool trailing_comma = true) {
    const char* comma = trailing_comma ? "," : "";
    if (format >= 0) {
        std::printf("      \"%s\": %d%s\n", key, format, comma);
        return;
    }
    std::printf("      \"%s\": null%s\n", key, comma);
}

void print_attempt(const Attempt& attempt, bool trailing_comma) {
    std::printf("    {\n");
    std::printf("      \"case\": \"%s\",\n", attempt.name);
    std::printf("      \"requested_color_format\": \"%s\",\n", attempt.requested.c_str());
    std::printf("      \"configure_ok\": %s,\n", attempt.configure_ok ? "true" : "false");
    std::printf("      \"configure_status\": \"%s\",\n", attempt.status.c_str());
    std::printf("      \"start_ok\": %s,\n", attempt.start_ok ? "true" : "false");
    print_format_field("format_before_start", attempt.format_before_start);
    print_format_field("format_after_start", attempt.format_after_start);
    if (attempt.note.empty()) {
        std::printf("      \"note\": null\n");
    } else {
        std::printf("      \"note\": \"%s\"\n", attempt.note.c_str());
    }
    std::printf("    }%s\n", trailing_comma ? "," : "");
}

}  // namespace

int main(int argc, char** argv) {
    if (argc < 2) {
        std::fprintf(stderr, "usage: %s <media-path>\n", argv[0]);
        return 2;
    }
    const std::string path = argv[1];

    const int fd = ::open(path.c_str(), O_RDONLY);
    if (fd < 0) {
        std::fprintf(stderr, "cannot open %s\n", path.c_str());
        return 3;
    }
    off_t length = ::lseek(fd, 0, SEEK_END);
    ::lseek(fd, 0, SEEK_SET);
    if (length <= 0) {
        std::fprintf(stderr, "empty file %s\n", path.c_str());
        ::close(fd);
        return 3;
    }

    mediacodec::MediaExtractorSource source;
    const auto opened = source.open_fd(fd, 0, static_cast<std::int64_t>(length), path);
    if (!opened.ok()) {
        std::fprintf(stderr, "open_fd failed: %s\n", opened.message.c_str());
        ::close(fd);
        return 4;
    }
    const mediacodec::VideoTrackInfo& info = source.track_info();
    const AMediaFormat* track_format = source.track_format();

    std::int32_t profile = -1;
    if (AMediaFormat_getInt32(const_cast<AMediaFormat*>(track_format), "profile", &profile)) {
        // 拿到了，记下来
    }
    std::int32_t color_standard = -1;
    std::int32_t color_transfer = -1;
    std::int32_t color_range = -1;
    AMediaFormat_getInt32(const_cast<AMediaFormat*>(track_format), "color-standard", &color_standard);
    AMediaFormat_getInt32(const_cast<AMediaFormat*>(track_format), "color-transfer", &color_transfer);
    AMediaFormat_getInt32(const_cast<AMediaFormat*>(track_format), "color-range", &color_range);

    std::vector<Attempt> attempts;
    // A：请求什么都不设 —— 平台默认。这是"基准"，没有它，B 失败就分不清是谁的问题。
    attempts.push_back(try_configure(info.mime, track_format, -1, "default_no_color_format"));
    // B：请求 P010。这张卡真正要问的那一下。
    attempts.push_back(try_configure(info.mime, track_format, color_format::kYuvP010,
                                     "request_p010"));
    // C：请求 Surface 但不给窗口 —— **反证**。它也会失败，
    //    所以"B 失败"不能直接读成"不支持 10bit"。
    Attempt surface_case = try_configure(info.mime, track_format, color_format::kSurface,
                                         "request_surface_without_window");
    // 这个 note **由观测结果决定**，不是写死的预期。
    // 第一版写的是"预期会失败，别读成不支持 10bit" —— 结果它成功了，
    // 那句 note 就成了假话。现在改成就事论事：
    surface_case.note = surface_case.configure_ok
        ? "configure accepted a Surface request with no window, so configure does NOT validate "
          "color-format; 'configure ok' is therefore not evidence of any format support"
        : "configure rejected Surface without a window (unexpected; record and investigate)";
    attempts.push_back(surface_case);

    std::printf("{\n");
    std::printf("  \"schema\": \"%s\",\n", kSchema);
    // 证据必须带实测时间戳：没有它就分不清"真机产物"和"手写片段"（F4 层 1 的判据）。
    const auto stamp_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                              std::chrono::system_clock::now().time_since_epoch())
                              .count();
    std::printf("  \"generated_at_unix_ms\": %lld,\n", static_cast<long long>(stamp_ms));
    std::printf("  \"media\": \"%s\",\n", path.c_str());
    std::printf("  \"mime\": \"%s\",\n", info.mime.c_str());
    std::printf("  \"width\": %d,\n", info.width);
    std::printf("  \"height\": %d,\n", info.height);
    std::printf("  \"source_profile\": %d,\n", profile);
    std::printf("  \"source_color_standard\": %d,\n", color_standard);
    std::printf("  \"source_color_transfer\": %d,\n", color_transfer);
    std::printf("  \"source_color_range\": %d,\n", color_range);
    std::printf("  \"attempts\": [\n");
    for (std::size_t i = 0; i < attempts.size(); ++i) {
        print_attempt(attempts[i], i + 1 < attempts.size());
    }
    std::printf("  ],\n");

    // ── 结论 ──
    // 判据只有一条：**start 之后**读到的格式，三个请求是不是同一个。
    // 若三个请求都给同一个值，说明这些请求键根本没被采纳 —— 那"请求 P010"这件事
    // 在这颗 codec 上就没有可观测的效果，不能拿它当"支持/不支持"的证据。
    const std::int32_t a = attempts[0].format_after_start;
    const std::int32_t b = attempts[1].format_after_start;
    const bool all_started = attempts[0].start_ok && attempts[1].start_ok;
    const bool same = all_started && a == b && a >= 0;
    std::printf("  \"verdict\": {\n");
    std::printf("    \"all_attempts_started\": %s,\n", all_started ? "true" : "false");
    std::printf("    \"same_format_for_all_requests\": %s,\n", same ? "true" : "false");
    std::printf("    \"format_after_start\": %s,\n",
                a >= 0 ? std::to_string(a).c_str() : "null");
    if (same) {
        std::printf(
            "    \"reading\": \"every request produced the same output format after start; "
            "this component does not honour color-format requests, so neither P010 support nor "
            "its absence can be concluded from this probe\"\n");
    } else {
        std::printf(
            "    \"reading\": \"requests produced different output formats; compare the "
            "format_after_start values above per case\"\n");
    }
    std::printf("  },\n");

    // 这句写死在产物里：它是最容易被误读的地方。
    std::printf("  \"scope\": \"this probe configures WITHOUT a window (ByteBuffer mode) and "
                "does not prove what the Surface playback path negotiates\"\n");
    std::printf("}\n");

    ::close(fd);
    return 0;
}
