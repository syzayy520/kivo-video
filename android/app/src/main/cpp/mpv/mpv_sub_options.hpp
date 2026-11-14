#pragma once

// 253 家族：字幕这一组 mpv 选项 —— **只做一件事**：把"字幕用哪个字体、怎么摆"翻译成 mpv 选项。
//
// 【为什么单独成文件】字幕这一组的**内容**会随"最小字体集"这件事一起变
//（字体目录从整个 /system/fonts 变成只含一两个文件的应用私有目录），
// 而视频/音频/缓存那组选项与字幕无关 —— 放一起会让"改字体"变成"改缓存选项的文件"。
//
// 【家族边界】B1~B5 见 mpv_session.hpp 顶部；本文件不引用旧内核、不碰 UI。

#include <string>
#include <vector>

struct mpv_handle;

namespace kivo::video::mpv {

/// 设好字幕这一组选项。
///
/// @param handle      mpv 句柄（未 initialize）。
/// @param fonts_dir   字体目录；**空 = 不设任何字体选项**
///                    （预览内核不起字幕，libass 从不初始化，不该背这份内存）。
/// @param font_family 字体族名；空 = 用 mpv 默认族。
/// @param failures    出参：失败项的描述，逐条追加。
/// @return true = 非 optional 项全部成功（optional 项的失败仍会进 failures，不静默）。
bool apply_sub_options(mpv_handle* handle,
                        const std::string& fonts_dir,
                        const std::string& font_family,
                        std::vector<std::string>& failures);

}  // namespace kivo::video::mpv
