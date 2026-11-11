// 253 家族：mpv 选项设置的实现 —— **只做一件事**：设选项，并逐项检查回执。
//
// 【本文件存在的唯一理由】
// mpv_set_option_string 对**不存在的选项名**返回 MPV_ERROR_OPTION_NOT_FOUND，
// 但**不打印任何日志**（见 mpv 的 player/client.c：只有返回值，没有消息）。
// 所以一个拼错的选项会完全静默 —— 调用方若不检查返回值，就永远不知道。
// 本文件把"设 + 检查"绑成一个动作，让静默失败不可能发生。
//
// 【选项名的来源】
// mpv 0.39 的字幕/字体类选项是**子结构前缀**形式，定义在 mpv 的 sub/osd.c：
//   style_opts = { "font", "font-size", "fonts-dir", "margin-y", "align-y", ... }
// 经 options.c 的 {"sub", OPT_SUBSTRUCT(...)} 与 {"osd", OPT_SUBSTRUCT(...)} 展开后，
// 可用的名字是：
//   sub-font / sub-font-size / sub-fonts-dir / sub-margin-y / sub-align-y
//   osd-font / osd-font-size / osd-fonts-dir / osd-margin-y / osd-align-y
// 而**不是** "sub-fonts-dir" 之外的其它写法 —— 例如 "osd-font" 是对的，
// "sub-fonts-dir" 也是对的，但 "sub-fonts-dir" 之外的拼法（例如把 dir 写成 dirs）会静默失败。
// 每个名字都在 mpv 的 DOCS/man/options.rst 里有对应条目，可逐条核对。
//
// 【"osd-dimensions" 不是选项】
// 它在 mpv 里只是**只读属性**（player/command.c 的 mp_property_osd_dim），
// 报告"上一次已知的 OSD 尺寸与到视频的边距"。没有 SET 分支，
// 用 set_option 写它会得到 MPV_ERROR_OPTION_NOT_FOUND。
// 想看它，只能用 mpv_get_property_string 读 —— 本文件末尾提供了那个读法。

#include "mpv_options.hpp"

#include <cstdio>
#include <string>
#include <vector>

#include "mpv/client.h"

namespace kivo::video::mpv {
namespace {

/// 一个待设的选项。
struct Option {
    const char* name;
    const char* value;
    /// 设为 true 时，**这一项失败不算整体失败**（只记录）。
    /// 用于"锦上添花"的选项 —— 例如字体族名不匹配时，字幕仍可能靠内嵌字体渲染。
    bool optional;
};

/// 把一行文本追加到日志文件。
///
/// 【为什么需要它】mpv 对不存在的选项**只返回错误码、不打日志**。
/// 若上层把异常吞掉（Kotlin 侧的 initialize 曾经就是这样），
/// 失败原因就彻底消失了 —— 于是排查只能靠猜。
/// 直接写进 mpv 的日志文件，是唯一"谁都能看见"的地方。
void append_log(const std::string& log_file, const std::string& line) {
    if (log_file.empty()) {
        return;
    }
    FILE* file = std::fopen(log_file.c_str(), "a");
    if (file == nullptr) {
        return;
    }
    std::fputs(line.c_str(), file);
    std::fputc('\n', file);
    std::fclose(file);
}

/// 逐项设置并检查回执。
///
/// @param failures 出参：失败项的描述（"name=value (reason)"），逐条追加。
/// @return true = 全部非 optional 项成功。
bool apply_options(mpv_handle* handle, const std::vector<Option>& options,
                   std::vector<std::string>& failures) {
    bool all_ok = true;
    for (const Option& option : options) {
        const int rc = mpv_set_option_string(handle, option.name, option.value);
        if (rc >= 0) {
            continue;
        }
        const char* reason = mpv_error_string(rc);
        failures.push_back(std::string(option.name) + "=" + option.value + " (" +
                           (reason != nullptr ? reason : "unknown") + ")");
        if (!option.optional) {
            all_ok = false;
        }
    }
    return all_ok;
}

}  // namespace

bool apply_mpv_options(mpv_handle* handle,
                       const std::string& vo,
                       const std::string& ao,
                       const std::string& log_file,
                       std::string& hwdec_out,
                       std::string& error) {
    if (handle == nullptr) {
        error = "mpv handle is null";
        return false;
    }

    hwdec_out.clear();
    std::vector<std::string> failures;

    // ---- 1) 日志：先把日志打开，后面任何失败的现场才有地方落 ----
    //
    // 【为什么放在最前面】mpv 的日志是**唯一**能看到"选项被拒绝"的渠道 ——
    // mpv_set_option_string 自己不打印。所以必须先有日志，再设其它选项。
    std::vector<Option> options;
    if (!log_file.empty()) {
        options.push_back({"log-file", log_file.c_str(), false});
        // 【为什么是 "all=debug" 而不是 "all"】
        // msg-level 是 OPT_MSGLEVELS —— 一个 **keyvalue 列表**（`模块=级别`），
        // 不是单值。裸的 "all" 会被判成非法级别名，返回 MPV_ERROR_OPTION_ERROR。
        // 级别名取自 mpv 的 common/msg.c：fatal/error/warn/info/status/v/debug/trace/stats。
        // 用 "all=debug" 打开全量消息 —— 字幕字体相关的信息只在 debug 级。
        //
        // 【这个 bug 藏了很久】旧代码写的是 msg-level=all，被 mpv 拒绝、且**不打印任何日志**
        // （mpv 对被拒的选项只回错误码）。于是 mpv 日志一直不是全量，
        // 字体匹配失败的信息压根没落盘 —— 上一轮因此查不到"为什么字幕不上屏"。
        // 本轮加了回执检查，第一轮就把它抓了出来。
        options.push_back({"msg-level", "all=debug", false});
    }

    // ---- 2) 视频/音频通路 ----
    options.push_back({"vo", vo.c_str(), false});
    // 硬解只对 mpv-android 明确支持的编码启用 —— 与 mpv-android 的
    // hwdec-codecs="h264,hevc,mpeg4,mpeg2video,vp8,vp9,av1" 保持一致。
    // VC-1 / TrueHD 这类平台没有硬解的，走 FFmpeg 软解（mpv 内部自动回退）。
    options.push_back({"hwdec-codecs", "h264,hevc,mpeg4,mpeg2video,vp8,vp9,av1", false});
    // 音频输出：ao 由调用方给；空 = "null"（不出声）。
    options.push_back({"ao", ao.empty() ? "null" : ao.c_str(), false});
    // video-sync=audio：音画以音频为主时钟 —— 与旧内核的"音频主时钟"一致。
    options.push_back({"video-sync", "audio", false});
    // hr-seek=default：精确落位交给 mpv 自己的 demuxer（索引 + 帧级回退）。
    options.push_back({"hr-seek", "default", false});
    // demuxer 缓存上限：mpv-android 在移动端把它压到 32~64 MB
    // （"defaults are too high for mobile devices"）。我们照做 ——
    // 缓存太大在 SMB 上会拖慢起播，且占内存。
    options.push_back({"demuxer-max-bytes", "64MiB", false});
    options.push_back({"demuxer-max-back-bytes", "64MiB", false});

    // ---- 3) 字幕（253-P6-2）----
    //
    // 【为什么指向 /system/fonts】
    // 我们编的 libass 是 CONFIG_FONTCONFIG 未定义的版本（见
    // libass-android 的 config.h），也就是**没有 fontconfig 字体提供器**。
    // mpv 的 sub/osd.c 会把 fonts_dir 交给 libass 的 ass_set_fonts_dir，
    // 于是系统字体目录里的字体文件会被逐个 ass_add_font 装进来。
    // 不给目录 ⇒ 中文字幕全是空白或豆腐块。
    //
    // 【字体族名为什么给 "Noto Sans CJK JP" 而不是 SC】
    // libass 加载字体目录时逐个文件调 ass_add_font，而 ass_add_font 对
    // **TrueType Collection 只取 0 号 face**（libass 的 ass_font.c：
    // FT_Open_Face(ftlib, &args, index, &face) 的 index 来自选中的那一张）。
    // Android 的 NotoSansCJK-Regular.ttc 里 0 号 face 是 JP。
    // 字形层面无损失：CJK 全字集每个 face 都带，中文照样显示。
    //
    // 【osd-font 与 sub-font 都要设】
    // mpv 把字幕转成 ASS 事件后**由 OSD 层统一渲染**（osd_render），
    // 而 osd_render 取的是 **osd_style** 的字体设置 —— 不是 sub_style。
    // 只设 sub-font 时日志仍打印 (sans-serif, 400, 0)，即完全没生效。
    //
    // 【optional = true 的理由】
    // 字体族名匹配不上时，mpv 会退回 libass 的默认字体族（可能仍能显示，
    // 只是字形不对）。这属于"降级但可见"，不该让内核整个起不来 ——
    // 但**必须被记录**，否则又变成静默失败。
    options.push_back({"sub-fonts-dir", "/system/fonts", true});
    options.push_back({"osd-font", "Noto Sans CJK JP", true});
    options.push_back({"sub-font", "Noto Sans CJK JP", true});
    // 字号：mpv 默认按视频高度的比例算字号。
    // sub-scale 是百分比因子且会破坏 ASS 缩放，sub-font-size 是绝对字号，
    // 只对纯文本字幕的默认字号生效。52 px 在 1080 高的画面上约等于 5%。
    options.push_back({"sub-font-size", "52", true});
    // 自动加载同名字幕文件（外挂 .srt/.ass 放在媒体旁边）。
    options.push_back({"sub-auto", "fuzzy", true});
    // ---- 位图字幕（PGS）的显示时长 ----
    //
    // 【为什么必须开】PGS 是**渐进式位图字幕**：一句对白被拆成一串"更新包"，
    // 每个包只带画面的一小块更新。实测 Shelter 的中文 PGS 轨：
    //   包区间 [13.639..13.680] [13.847..13.889] [14.056..14.097] …
    //   ⇒ 每个包只有 **41 ms** 宽，间隔约 **208 ms**。
    //
    // 原因在 mpv 的 sub/sd_lavc.c:
    //   endpts = pts + sub.end_display_time / 1000.0;
    // FFmpeg 的 pgssub 解码器把 end_display_time 设为 0（PGS 没有"结束时间"，
    // 它用 composition 的 display_end），于是每个包只覆盖自己那 41 ms。
    //
    // 而 mpv 的 get_current 要求 `pts < sub->endpts` —— 严格落在包内才命中。
    // 于是**包与包之间的 208 ms 间隙里没有字幕**，一句完整对白被切成断断续续的碎片。
    //
    // 【--sub-fix-timing 正是为此存在】官方说明：
    //   "Adjust subtitle timing to remove minor gaps or overlaps between
    //    subtitles (if the difference is smaller than 210 ms, the gap or
    //    overlap is removed)."
    // 阈值 210 ms 与实测的 208 ms 间隔刚好吻合 ⇒ 开启后间隙被填平，
    // 一句对白变成连续显示。
    //
    // 【为什么设成 optional】它只影响字幕的时间轴，不影响能否起播；
    // 但它对位图字幕是**必需**的，所以失败必须被记录（不许静默降级）。
    options.push_back({"sub-fix-timing", "yes", true});

    const bool ok = apply_options(handle, options, failures);

    if (!ok) {
        std::string message = "mpv option rejected: ";
        for (std::size_t i = 0; i < failures.size(); ++i) {
            if (i != 0) {
                message += "; ";
            }
            message += failures[i];
        }
        error = message;
        // 【为什么还要写日志】失败可能发生在上层把异常吞掉的地方
        // （Kotlin 的 initialize 曾经就吞了）。日志文件是唯一"一定看得见"的地方 ——
        // 真机诊断时，第一件事就是读它。
        append_log(log_file, "OPTFAIL " + message);
        return false;
    }

    // 全部成功时，optional 项的失败（若有）也记下来 —— 它们不影响起播，
    // 但必须留痕（铁律：不许静默降级）。
    if (!failures.empty()) {
        std::string message = "mpv option degraded: ";
        for (std::size_t i = 0; i < failures.size(); ++i) {
            if (i != 0) {
                message += "; ";
            }
            message += failures[i];
        }
        error = message;
        append_log(log_file, "OPTWARN " + message);
    }

    // 硬解驱动名：**必须在 initialize 之后**用 set_property 设。
    // 在 initialize 之前用 set_option 设会被 mpv 用默认值覆盖
    // （真机日志：明明设了 mediacodec-copy，mpv 仍去加载 aimagereader 驱动）。
    hwdec_out = "mediacodec-copy";
    return true;
}

}  // namespace kivo::video::mpv
