// 253 家族：字幕选项组 —— **只做一件事**：把"字幕用哪个字体、怎么摆"翻译成 mpv 选项。
//
// 【为什么字体要占一个"目录"而不是"一个文件"】
// libass 认的是**目录**（ass_set_fonts_dir）：它对该目录里的**每个条目**调一次
// ass_add_font()，而 FreeType 的 FT_New_Face 会把整个文件读进内存。
// 所以"只用一种字体"这件事，在 libass 那里只能靠"目录里只有那一种字体"实现。
//
// 【真机代价（273 v20，Redmi K40）】
// sub-fonts-dir=/system/fonts ⇒ 内核日志 242 条 "Loading font file"
// ⇒ 239 个字体文件（磁盘合计 125.6 MB）**全部常驻原生堆**。
// 改成只含一两个文件的私有目录后，常驻量降到 ~22 MB。
//
// 【为什么 osd-font 与 sub-font 都要设】
// mpv 把字幕转成 ASS 事件后**由 OSD 层统一渲染**（osd_render），
// 而 osd_render 取的是 **osd_style** 的字体设置 —— 不是 sub_style。
// 只设 sub-font 时日志仍打印 (sans-serif, 400, 0)，即完全没生效。

#include "mpv_sub_options.hpp"

#include "mpv_option_item.hpp"

namespace kivo::video::mpv {
namespace {

/// 族名兜底：上层给不出族名时用这个（Noto Sans CJK 的 0 号 face 族名）。
const char* kDefaultFamily = "Noto Sans CJK JP";

}  // namespace

bool apply_sub_options(mpv_handle* handle,
                        const std::string& fonts_dir,
                        const std::string& font_family,
                        std::vector<std::string>& failures) {
    if (handle == nullptr) {
        failures.push_back("sub-fonts-dir=(null handle)");
        return false;
    }
    // 预览内核：不起字幕 ⇒ libass 从不初始化 ⇒ 一个字体都不该加载。
    // 【为什么这里返回 true 而不是设 /system/fonts】那会让预览内核也背上
    // 125.6 MB 的字体常驻，而它一帧字幕都不渲染（真机日志：预览内核 0 条
    // "Loading font file"，因为 sid=no）。
    if (fonts_dir.empty()) {
        return true;
    }

    const std::string dir = fonts_dir;
    const std::string family = font_family.empty() ? std::string(kDefaultFamily) : font_family;

    std::vector<Option> options;
    // 字体目录：libass 逐文件装载（目录里有多少文件，就常驻多少内存）。
    //
    // 【为什么 optional】目录不存在时 mpv 仍会接受这个选项，只是找不到字体。
    // 真正的失败在更上游 —— 上层保证目录存在（见 Kotlin 的 SubtitleFontSet）。
    // 但失败仍会进 failures ⇒ 不静默。
    options.push_back({"sub-fonts-dir", dir.c_str(), true});
    options.push_back({"osd-font", family.c_str(), true});
    options.push_back({"sub-font", family.c_str(), true});
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

    return apply_options(handle, options, failures);
}

}  // namespace kivo::video::mpv
