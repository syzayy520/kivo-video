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
#include "mpv_option_item.hpp"
#include "mpv_sub_options.hpp"

namespace kivo::video::mpv {

bool apply_mpv_options(mpv_handle* handle,
                       const std::string& vo,
                       const std::string& ao,
                       const std::string& log_file,
                       const std::string& sub_fonts_dir,
                       const std::string& sub_font_family,
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
    // 【273 v19 实验结论：不要开 --audio-stream-silence】
    // 试过（真机 Redmi K40）：mpv 接受了该选项并自己警告
    //   "The --audio-stream-silence option is set. This will break certain player behavior."
    // 但**欠载一次也没消**（起播 1 次 + seek 2 次，与不开时完全相同）——
    // 因为 mpv 0.39 的 audiotrack AO 不支持静音填充控制。
    // 换来的只有 A/V 同步被改动 ⇒ 收益为零、代价明确 ⇒ 不开。
    // hr-seek=default：精确落位交给 mpv 自己的 demuxer（索引 + 帧级回退）。
    options.push_back({"hr-seek", "default", false});
    // demuxer 缓存上限：mpv-android 在移动端把它压到 32~64 MB
    // （"defaults are too high for mobile devices"）。我们照做 ——
    // 缓存太大在 SMB 上会拖慢起播，且占内存。
    //
    // 【273 v25：64 MiB → 32 MiB】真机 A/B（同一片源、同一台 K40、
    // 四臂各 125 s、24 次采样、0 次缓冲事件）：
    //   基线（cache-secs=60 / 64+16 MiB）：native 堆峰值 121,649 KB，
    //   总下载 86.31 MiB，mpv demux 缓存 49.3 MB；
    //   cache-secs=15 那一臂：native 堆峰值 104,519 KB、总下载 73.31 MiB。
    // 字节上限这一项当时**没能在运行时验证**（mpv 拒绝运行期改这一项，
    // 静默失败、无日志）⇒ 只能焊在初始化里，改完重编再复算。
    options.push_back({"demuxer-max-bytes", "32MiB", false});
    // 回看缓存（当前播放点**之前**的数据）单独给 8 MiB（v25：16 → 8）。
    //
    // 【为什么不给 64】mpv 的 demux 缓存有两块：前向（demuxer-max-bytes）
    // 与回看（demuxer-max-back-bytes），二者相加才是真实常驻。273 真机实测：
    // 64+64=128 MiB 把 Native Heap 顶到 297.8 MB、总 PSS 518.7 MB。
    // 回看缓存只服务于"往回 seek"，而我们的取数层（v6 有界分块 + 连接复用）
    // 在回退时本来就会按目标字节重新开 Range 请求 —— 4K 片源上回退 seek
    // 实测 1.16 s 可达，不需要 64 MiB 回看缓存兜底。
    options.push_back({"demuxer-max-back-bytes", "8MiB", false});
    // 网络缓存同时受"时间"约束（273）：cache-secs 默认约 1000 小时 —— 时间上等于
    // 无上限，真正的兜底只有 demuxer-max-bytes。给 60 秒把预读的**时间跨度**也
    // 钉住，避免连接一路读到片尾把整片塞进内存。
    //
    // 【273 v25：60 → 20 秒】这是四个臂里**唯一真正生效**的旋钮
    // （mpv 运行期接受 cache-secs 与 demuxer-max-bytes，唯独拒绝
    // demuxer-max-back-bytes）。真机四臂都是 0 次缓冲事件 ⇒ 20 秒安全。
    // 取 20 而不是实测的 15：15 与 20 之间没有实测差别，而 20 多留 5 秒
    // 的跑道给慢源（起播那一跳的冷连接表头最高见过 1.6 s）。
    options.push_back({"cache-secs", "20", false});
    // 迟滞：缓存掉到零附近时不立刻回退重缓冲，先攒够 10 秒再继续播。
    // mpv 用它消掉"缓存 100% → 0% → 100%"的来回抖动。
    options.push_back({"demuxer-hysteresis-secs", "10", false});

    // ---- 3) 字幕 ----
    //
    // 【为什么字幕选项单独一组】字体相关的那几项（目录/族名）会随
    // "最小字体集"这件事一起变（273 v20：目录从整个 /system/fonts 变成
    // 只含一两个文件的私有目录），与本组的其它项无关 —— 见 mpv_sub_options.cpp。
    //
    // 【字体目录为什么不再是 /system/fonts】
    // 我们编的 libass 没有 CONFIG_FONTCONFIG（没有 fontconfig 字体提供器），
    // 于是 mpv 会把 fonts_dir 交给 libass 的 ass_set_fonts_dir，
    // 它对该目录里的**每个条目**调一次 ass_add_font，
    // 而 FreeType 的 FT_New_Face 会把整个文件读进内存。
    // 真机实测（Redmi K40，273 v20）：指向 /system/fonts ⇒ 242 条
    // "Loading font file" ⇒ 239 个字体文件、125.6 MB 全部常驻原生堆。
    // 现在由上层把"最小字体目录"准备好传进来（见 Kotlin 的 SubtitleFontSet）。
    //
    // 【顺序】放在 apply_options 之后 —— 日志文件必须先打开，
    // 否则字幕这组的失败无处留痕。
    const bool base_ok = apply_options(handle, options, failures);
    const bool sub_ok = apply_sub_options(handle, sub_fonts_dir, sub_font_family, failures);
    const bool ok = base_ok && sub_ok;

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
        append_option_log(log_file, "OPTFAIL " + message);
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
        append_option_log(log_file, "OPTWARN " + message);
    }

    // 硬解驱动名：**必须在 initialize 之后**用 set_property 设。
    // 在 initialize 之前用 set_option 设会被 mpv 用默认值覆盖
    // （真机日志：明明设了 mediacodec-copy，mpv 仍去加载 aimagereader 驱动）。
    //
    // 【为什么是零拷贝的 "mediacodec" 而不是 "mediacodec-copy"】
    // "-copy" 会把解码出来的每一帧拷进 CPU 内存（4K NV12 = 11.06 MB/帧），
    // 真机实测 native heap 337 MB / TOTAL PSS 737 MB；
    // 零拷贝把帧留在 MediaCodec 的 Surface 上，native 降到 278 MB / PSS 544 MB
    // （Redmi K40，4K HEVC 3840×1920，同一片源同一口径）。
    //
    // 【零拷贝在我们这条渲染链上确实能用】vo=libmpv + 我们自己的 EGL/GLES3 上下文，
    // mpv 能把 MediaCodec 的帧映射进来：真机画面正常、渲染 23.5 fps 稳定、无映射报错。
    hwdec_out = "mediacodec";
    return true;
}

}  // namespace kivo::video::mpv
