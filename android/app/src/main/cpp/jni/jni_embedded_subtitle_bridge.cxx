// jni_embedded_subtitle_bridge.cxx — AI 字幕翻译的内嵌文本轨提取（FFmpeg avformat）。
//
// 落位：android/app/src/main/cpp/jni/，与 jni_ffmpeg_bridge 平行。
// 家族：AI 字幕工具链的独立功能——不创建/触碰任何播放会话，只读源文件。
//
// 为什么不用 mpv sub-dump：本项目的 libmpv 构建没有 sub-dump 命令
// （player/command.c 与 sub/ 全树 grep 零命中，2026-11-22 实测 ch4b rejected）。
// 为什么用 FFmpeg：MKV 解封装是项目既定路线（平台解封装器不解析
// SeekHead→Cues），且与 SenPlayer 同架构。
//
// 支持：SUBRIP（MKV 内嵌 SRT）与 MOV_TEXT（MP4 tx3g）。其它字幕类型不计入
// 文本轨序号（与 Kotlin 的 isTextSubtitleCodec 同口径：只数可翻译的文本轨）。
//
// 无 FFmpeg 头（dist 缺失）时编空实现：返回 null + reason，不阻断构建。

#include <jni.h>

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <mutex>
#include <string>

#if __has_include("libavformat/avformat.h")
#define KIVO_EMBEDDED_EXTRACT_AVAILABLE 1
extern "C" {
#include "libavformat/avformat.h"
#include "libavcodec/avcodec.h"
#include "libavutil/avutil.h"
#include "libavutil/mathematics.h"
}
#endif

namespace {

std::mutex g_errorMutex;
std::string g_lastError;

void set_last_error(const std::string& reason) {
    std::lock_guard<std::mutex> lock(g_errorMutex);
    g_lastError = reason;
}

#ifdef KIVO_EMBEDDED_EXTRACT_AVAILABLE

std::string srt_timestamp(std::int64_t ms) {
    if (ms < 0) { ms = 0; }
    const std::int64_t totalSec = ms / 1000;
    char buf[32];
    std::snprintf(buf, sizeof(buf), "%02lld:%02lld:%02lld,%03lld",
                  static_cast<long long>(totalSec / 3600),
                  static_cast<long long>((totalSec % 3600) / 60),
                  static_cast<long long>(totalSec % 60),
                  static_cast<long long>(ms % 1000));
    return buf;
}

std::int64_t to_ms(const AVRational& tb, std::int64_t value) {
    if (value == AV_NOPTS_VALUE) { return -1; }
    return av_rescale_q(value, tb, AVRational{1, 1000});
}

std::string extract_embedded_subtitle(const std::string& path, int ordinal) {
    AVFormatContext* fmt = nullptr;
    if (avformat_open_input(&fmt, path.c_str(), nullptr, nullptr) < 0) {
        set_last_error("avformat_open_input 失败（路径/协议不可读）");
        return {};
    }
    int seen = 0;
    int target = -1;
    for (unsigned i = 0; i < fmt->nb_streams; ++i) {
        const auto* par = fmt->streams[i]->codecpar;
        if (par->codec_type != AVMEDIA_TYPE_SUBTITLE) { continue; }
        if (par->codec_id != AV_CODEC_ID_SUBRIP && par->codec_id != AV_CODEC_ID_MOV_TEXT) { continue; }
        if (seen++ == ordinal) { target = static_cast<int>(i); break; }
    }
    if (target < 0) {
        set_last_error("没有对应序号的文本字幕流");
        avformat_close_input(&fmt);
        return {};
    }
    const AVRational tb = fmt->streams[target]->time_base;
    const auto codecId = fmt->streams[target]->codecpar->codec_id;
    std::string out;
    int cue = 0;
    std::int64_t lastEndMs = 0;
    AVPacket* pkt = av_packet_alloc();
    while (av_read_frame(fmt, pkt) >= 0) {
        if (pkt->stream_index == target && pkt->size > 0) {
            std::string text;
            if (codecId == AV_CODEC_ID_MOV_TEXT) {
                if (pkt->size > 2) {
                    const auto* d = reinterpret_cast<const std::uint8_t*>(pkt->data);
                    const std::uint16_t len = static_cast<std::uint16_t>((d[0] << 8) | d[1]);
                    const size_t n = std::min<size_t>(len, static_cast<size_t>(pkt->size - 2));
                    text.assign(reinterpret_cast<const char*>(d + 2), n);
                }
            } else {
                text.assign(reinterpret_cast<const char*>(pkt->data), static_cast<size_t>(pkt->size));
            }
            if (!text.empty()) {
                std::int64_t startMs = to_ms(tb, pkt->pts);
                if (startMs < 0) { startMs = lastEndMs; }
                std::int64_t durMs = to_ms(tb, pkt->duration);
                if (durMs <= 0) { durMs = 2000; }
                const std::int64_t endMs = startMs + durMs;
                out += std::to_string(++cue) + "\n" +
                       srt_timestamp(startMs) + " --> " + srt_timestamp(endMs) + "\n" +
                       text + "\n\n";
                lastEndMs = endMs;
            }
        }
        av_packet_unref(pkt);
    }
    av_packet_free(&pkt);
    avformat_close_input(&fmt);
    if (cue == 0) {
        set_last_error("字幕流里没有文本包");
        return {};
    }
    return out;
}

#endif  // KIVO_EMBEDDED_EXTRACT_AVAILABLE

}  // namespace

extern "C" JNIEXPORT jstring JNICALL
Java_com_kivo_cinema_bridge_NativePlayer_nativeExtractEmbeddedSubtitle(
    JNIEnv* env, jobject /*thiz*/, jstring path, jint ordinal) {
#ifdef KIVO_EMBEDDED_EXTRACT_AVAILABLE
    if (path == nullptr) {
        set_last_error("媒体路径为空");
        return nullptr;
    }
    const char* chars = env->GetStringUTFChars(path, nullptr);
    if (chars == nullptr) {
        set_last_error("媒体路径编码读取失败");
        return nullptr;
    }
    const std::string mediaPath(chars);
    env->ReleaseStringUTFChars(path, chars);
    const std::string srt = extract_embedded_subtitle(mediaPath, static_cast<int>(ordinal));
    if (srt.empty()) { return nullptr; }
    // NewStringUTF 是 modified UTF-8：英文源安全；非 BMP 多语言源若引入需改 NewString。
    return env->NewStringUTF(srt.c_str());
#else
    (void)path;
    (void)ordinal;
    set_last_error("本构建未包含 FFmpeg（dist 缺失）");
    return nullptr;
#endif
}

extern "C" JNIEXPORT jstring JNICALL
Java_com_kivo_cinema_bridge_NativePlayer_nativeEmbeddedExtractLastError(
    JNIEnv* env, jobject /*thiz*/) {
    std::lock_guard<std::mutex> lock(g_errorMutex);
    return env->NewStringUTF(g_lastError.c_str());
}