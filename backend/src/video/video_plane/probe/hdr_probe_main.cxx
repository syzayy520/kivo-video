// hdr_probe_main.cxx — P12-E2：颜色标记到底有没有被下发、下发后平台认不认
//
// 为什么需要它：播放器走 Surface，输出格式里读不到位深（E1 已证），
// 但"我们写进 configure 格式的颜色键，平台有没有采纳"是可以用 A/B 两次 configure 问出来的：
//
//   A 原样传轨道格式  → 平台能读到的颜色键就是容器给的
//   B 复制再写颜色键  → 与 A 的差别只有"我们明确写了一遍"
//
// 两次都读 **start() 之后** 的输出格式（E1 的教训：configure 之后读到的是默认值）。
// 若 A/B 输出一致，结论只能是"这条路径上看不到差异"，**不能**说"下发没用"——
// 平台可能本来就从轨道格式里读到了同样的值。
//
// ⚠️ 本探针**不给窗口**（ByteBuffer 模式）。它的结论不自动适用于 Surface 播放路径，
// 产物里 `scope` 字段写死了这句话。
#include <dlfcn.h>
#include <fcntl.h>
#include <unistd.h>

#include <chrono>
#include <cstdio>
#include <string>

#include <media/NdkMediaCodec.h>
#include <media/NdkMediaFormat.h>

#include "video/video_plane/backends/mediacodec/decode/color_key_names.hpp"
#include "video/video_plane/backends/mediacodec/decode/mediaformat_color_io.hpp"
#include "video/video_plane/backends/mediacodec/decode/video_color_facts.hpp"
#include "video/video_plane/backends/mediacodec/source/mediaextractor_source.hpp"

namespace {

namespace mediacodec = kivo::video::mediacodec;
namespace color_key = kivo::video::mediacodec::color_key;

// /3（P12-F3）：加 `track_format_dump` —— 把平台给的全部键原样打出来。
// 为什么需要它：加密样本在设备上 `is_encrypted=false`，但那只是**我们读的那个位**。
// 要说"平台根本没给加密元数据"，得看轨道格式里到底有没有相关键；否则
// "平台不报"和"我们没读"永远分不开 —— 这条是 F3 的判据核心。
constexpr const char* kSchema = "kivo.p12e.hdr-probe/3";

/// `AMediaFormat_toString` 是 API 28 才导出的，minSdk = 26 —— 走 dlsym。
/// 并且**先 dlopen 再按句柄查**：E3 那次 `RTLD_DEFAULT` 返回 null 的坑
/// （符号不在 libandroid.so，在 libnativewindow.so）已经教过一遍了。
using FormatToStringFn = const char* (*)(AMediaFormat*);

const FormatToStringFn& format_to_string() {
    static const FormatToStringFn fn = [] {
        if (void* handle = dlopen("libmediandk.so", RTLD_NOW | RTLD_LOCAL)) {
            return reinterpret_cast<FormatToStringFn>(dlsym(handle, "AMediaFormat_toString"));
        }
        return static_cast<FormatToStringFn>(nullptr);
    }();
    return fn;
}

/// 把平台给的格式转成一行可读文本；拿不到就返回 `nullptr`（**不编造**）。
const char* dump_format(AMediaFormat* format) {
    const FormatToStringFn& fn = format_to_string();
    return (format != nullptr && fn != nullptr) ? fn(format) : nullptr;
}

/// JSON 字符串转义。格式 dump 里一定会有引号和反斜杠。
std::string escape_json(const char* raw) {
    std::string out;
    for (const char* c = raw; c != nullptr && *c != '\0'; ++c) {
        switch (*c) {
            case '"': out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            default: out += *c; break;
        }
    }
    return out;
}

struct OutputColor {
    bool read_ok = false;
    std::int32_t standard = -1;
    std::int32_t transfer = -1;
    std::int32_t range = -1;
};

struct Attempt {
    const char* name = "";
    bool configure_ok = false;
    std::string configure_status;
    bool start_ok = false;
    OutputColor output;
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

/// 读 `getOutputFormat()` 里的三个颜色键 + `color-format`。
///
/// **必须在 `start()` 之后调**：configure 之后读到的是默认值（E1 第一版就栽在这里）。
[[nodiscard]] OutputColor read_output_color(AMediaCodec* codec) {
    OutputColor out;
    AMediaFormat* format = AMediaCodec_getOutputFormat(codec);
    if (format == nullptr) {
        return out;
    }
    out.read_ok = true;
    std::int32_t value = -1;
    if (AMediaFormat_getInt32(format, color_key::kKeyColorStandard, &value)) {
        out.standard = value;
    }
    if (AMediaFormat_getInt32(format, color_key::kKeyColorTransfer, &value)) {
        out.transfer = value;
    }
    if (AMediaFormat_getInt32(format, color_key::kKeyColorRange, &value)) {
        out.range = value;
    }
    AMediaFormat_delete(format);
    return out;
}

/// 跑一次 configure + start，读协商后的颜色键。
///
/// @param format 喂给平台的格式；所有权仍在调用方
[[nodiscard]] Attempt run_attempt(const char* name, const std::string& mime,
                                  const AMediaFormat* format) {
    Attempt attempt;
    attempt.name = name;

    AMediaCodec* codec = AMediaCodec_createDecoderByType(mime.c_str());
    if (codec == nullptr) {
        attempt.configure_status = "createDecoderByType returned null";
        return attempt;
    }
    const media_status_t configured = AMediaCodec_configure(codec, format, nullptr, nullptr, 0);
    attempt.configure_status = status_name(configured);
    if (configured == AMEDIA_OK) {
        attempt.configure_ok = true;
        const media_status_t started = AMediaCodec_start(codec);
        if (started == AMEDIA_OK) {
            attempt.start_ok = true;
            attempt.output = read_output_color(codec);
            AMediaCodec_stop(codec);
        } else {
            attempt.note = "start failed: " + status_name(started);
        }
    }
    AMediaCodec_delete(codec);
    return attempt;
}

void print_optional_int(const char* key, std::int32_t value, const char* indent,
                        bool trailing_comma) {
    std::printf("%s\"%s\": ", indent, key);
    if (value < 0) {
        std::printf("null");
    } else {
        std::printf("%d", value);
    }
    std::printf("%s\n", trailing_comma ? "," : "");
}

void print_output(const OutputColor& color, bool trailing_comma) {
    std::printf("      \"output_read_ok\": %s,\n", color.read_ok ? "true" : "false");
    print_optional_int("output_color_standard", color.standard, "      ", true);
    print_optional_int("output_color_transfer", color.transfer, "      ", true);
    print_optional_int("output_color_range", color.range, "      ", trailing_comma);
}

void print_attempt(const Attempt& attempt, bool trailing_comma) {
    std::printf("    {\n");
    std::printf("      \"case\": \"%s\",\n", attempt.name);
    std::printf("      \"configure_ok\": %s,\n", attempt.configure_ok ? "true" : "false");
    std::printf("      \"configure_status\": \"%s\",\n", attempt.configure_status.c_str());
    std::printf("      \"start_ok\": %s,\n", attempt.start_ok ? "true" : "false");
    print_output(attempt.output, true);
    std::printf("      \"note\": ");
    if (attempt.note.empty()) {
        std::printf("null\n");
    } else {
        std::printf("\"%s\"\n", attempt.note.c_str());
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
    const off_t length = ::lseek(fd, 0, SEEK_END);
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
    AMediaFormat_getInt32(const_cast<AMediaFormat*>(track_format), "profile", &profile);

    const mediacodec::SourceColorTags tags =
        mediacodec::read_source_color_tags(track_format);

    // 分类只用源侧事实 —— 与播放器同一条纯函数，避免"探针说 HDR、播放器说 SDR"。
    mediacodec::VideoColorFacts facts;
    facts.mime = info.mime;
    facts.source_profile = profile;
    facts.source_color_standard = tags.standard;
    facts.source_color_transfer = tags.transfer;
    facts.source_color_range = tags.range;
    const mediacodec::HdrAssessment hdr = mediacodec::classify_hdr(facts);

    // A：原样传轨道格式（基准）
    const Attempt baseline = run_attempt("track_format_as_is", info.mime, track_format);

    // B：复制再写颜色键（"下发"这个动作本身）
    AMediaFormat* explicit_format = mediacodec::make_configure_format(track_format, tags);
    Attempt delivered;
    if (explicit_format == nullptr) {
        delivered.name = "explicit_color_write";
        delivered.configure_status = "failed to build the explicit format";
    } else {
        delivered = run_attempt("explicit_color_write", info.mime, explicit_format);
        AMediaFormat_delete(explicit_format);
    }

    // C（P12-E4）：在 B 的基础上请求"输出转 SDR"。
    // 与 E1 试过的 `color-format` 是两条独立的键：那颗组件不校验 color-format，
    // 但认 color-* 三键 —— 所以"能不能让它转 SDR"必须单独问一次。
    AMediaFormat* sdr_request_format = mediacodec::make_configure_format_with_transfer_request(
        track_format, tags, color_key::kTransferSdrVideo);
    Attempt sdr_requested;
    if (sdr_request_format == nullptr) {
        sdr_requested.name = "request_sdr_transfer";
        sdr_requested.configure_status = "failed to build the explicit format";
    } else {
        sdr_requested = run_attempt("request_sdr_transfer", info.mime, sdr_request_format);
        AMediaFormat_delete(sdr_request_format);
    }

    // ── 结论 ──
    // 只比 start 之后读到的三个键。相等只能在报告里写"看不到差异"，
    // **不能**升格成"下发无效"——A 组可能本来就已经读到了同样的值。
    const bool comparable = baseline.start_ok && delivered.start_ok;
    const bool same_color = comparable &&
                            baseline.output.standard == delivered.output.standard &&
                            baseline.output.transfer == delivered.output.transfer &&
                            baseline.output.range == delivered.output.range;

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
    print_optional_int("source_profile", profile, "  ", true);
    print_optional_int("source_color_standard", tags.standard, "  ", true);
    print_optional_int("source_color_transfer", tags.transfer, "  ", true);
    print_optional_int("source_color_range", tags.range, "  ", true);
    print_optional_int("hdr_static_info_bytes", tags.static_info_bytes, "  ", true);
    std::printf("  \"hdr_kind\": \"%s\",\n", mediacodec::hdr_kind_name(hdr.kind));
    std::printf("  \"hdr_kind_reason\": \"%s\",\n", hdr.reason.c_str());
    // P12-F3：轨道格式全量 dump。加密样本判据靠它 —— 平台若给了加密键，这里一定会出现。
    std::printf("  \"track_format_dump\": ");
    const char* format_text = dump_format(const_cast<AMediaFormat*>(track_format));
    if (format_text == nullptr) {
        std::printf("null,\n");
    } else {
        std::printf("\"%s\",\n", escape_json(format_text).c_str());
    }

    // P12-F3：把"第一包读到的加密标志"打出来。
    // 背景：CENC 样本在 K40 上被解出垃圾帧却 completed=true —— 若 `is_encrypted` 一直是 false，
    // 说明"拒绝加密样本"那条分支在这台设备上根本不可达，必须写进证据而不是靠猜。
    mediacodec::SampleView first_sample;
    const auto first_read = source.read_next_sample(first_sample);
    std::printf("  \"first_sample\": {\n");
    std::printf("    \"read_ok\": %s,\n", first_read == mediacodec::SampleReadStatus::kOk ? "true" : "false");
    std::printf("    \"is_encrypted\": %s,\n", first_sample.is_encrypted ? "true" : "false");
    std::printf("    \"is_sync\": %s,\n", first_sample.is_sync ? "true" : "false");
    std::printf("    \"size\": %d\n", static_cast<int>(first_sample.size));
    std::printf("  },\n");

    std::printf("  \"attempts\": [\n");
    print_attempt(baseline, true);
    print_attempt(delivered, true);
    print_attempt(sdr_requested, false);
    std::printf("  ],\n");
    const bool sdr_request_started = sdr_requested.start_ok;
    const bool sdr_request_honoured =
        sdr_request_started && sdr_requested.output.transfer == color_key::kTransferSdrVideo;
    std::printf("  \"verdict\": {\n");
    std::printf("    \"both_started\": %s,\n", comparable ? "true" : "false");
    std::printf("    \"same_color_after_start\": %s,\n",
                comparable ? (same_color ? "true" : "false") : "null");
    if (!comparable) {
        std::printf(
            "    \"reading\": \"at least one attempt failed to start; no comparison can be made\"\n");
    } else if (same_color) {
        std::printf(
            "    \"reading\": \"explicitly writing the color keys produced the same reported "
            "color as passing the track format through; this path shows no observable effect, "
            "which does not mean the delivery was ignored — the platform may already read the "
            "same keys from the track format\"\n");
    } else {
        std::printf(
            "    \"reading\": \"explicitly writing the color keys changed what the decoder "
            "reports after start; compare per-key values above\"\n");
    }
    std::printf("  },\n");
    // P12-E4：这一条决定 tone mapping 走哪条路 —— 只看 start 之后的输出 transfer。
    std::printf("  \"sdr_transfer_request\": {\n");
    std::printf("    \"requested_transfer\": %d,\n", color_key::kTransferSdrVideo);
    std::printf("    \"start_ok\": %s,\n", sdr_request_started ? "true" : "false");
    std::printf("    \"output_transfer\": %s,\n",
                sdr_requested.output.transfer >= 0
                    ? std::to_string(sdr_requested.output.transfer).c_str()
                    : "null");
    std::printf("    \"honoured\": %s,\n", sdr_request_honoured ? "true" : "false");
    std::printf("    \"reading\": \"%s\"\n",
                sdr_request_honoured
                    ? "the decoder reported SDR transfer after start, so it accepts the SDR "
                      "output request; whether the pixels were actually tone-mapped still needs a "
                      "screenshot comparison"
                    : "the decoder kept the source transfer after start, so the SDR output request "
                      "was ignored; decoder-side tone mapping is not available on this component");
    std::printf("  },\n");
    std::printf("  \"scope\": \"this probe configures WITHOUT a window (ByteBuffer mode); it does "
                "not prove what the Surface playback path negotiates\"\n");
    std::printf("}\n");

    ::close(fd);
    return 0;
}
