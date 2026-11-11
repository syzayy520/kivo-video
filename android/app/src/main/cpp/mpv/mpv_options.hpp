#pragma once

// 253 家族：mpv 选项设置 —— **只做一件事**：把"我们想要的 mpv 行为"翻译成 mpv 选项，
// 并且**逐项检查 mpv 的回执**。
//
// 【为什么单独一个文件（家族树 §二的"差异记录"）】
// 初稿把选项并进 mpv_session.cpp，理由是"选项只有七八个"。本轮（P6 复盘）证明
// 那个判断错了 —— 不是选项多，而是**选项设错了没人知道**：
// mpv_set_option_string 对不存在的选项返回 MPV_ERROR_OPTION_NOT_FOUND，
// 而 mpv_session.cpp 当时**丢弃了全部返回值**，于是：
//   · 4 个拼错的选项（sub-fonts-dir 等）静默失败；
//   · 排查的人看到"选项设了但没生效"，只能靠猜。
// 选项设置因此成为**一件独立的事**：它有自己的失败模式、自己的判据、自己的证据。
//
// 【本文件的判据】
// 每个选项都必须**要么生效、要么报错**。不允许静默失败 ——
// 这条与项目铁律"不许静默降级"是同一件事。
//
// 【家族边界】B1~B5 见 mpv_session.hpp 顶部；本文件不引用旧内核、不碰 UI。

#include <string>

struct mpv_handle;

namespace kivo::video::mpv {

/// 把一整组 mpv 选项设到句柄上。
///
/// @param handle      mpv 句柄（未 initialize）。
/// @param vo          视频输出名。
/// @param ao          音频输出名；空 = "null"（不出声）。
/// @param log_file    mpv 日志落盘路径；空 = 不落盘。
/// @param hwdec_out   出参：硬解驱动名。**必须在 initialize 之后**用 set_property 设
///                    （原因见 mpv_options.cpp 的说明），所以这里只把值带出去。
/// @param error       出参：第一个失败的选项（"name: reason"）。全部成功时为空。
/// @return true = 全部选项都生效；false = 至少一个被 mpv 拒绝。
///
/// 【为什么"一个失败就整体失败"】
/// mpv 的选项是"要么这个行为、要么默认行为"。设不上就等于默默用了默认行为，
/// 而默认行为可能恰好是我们要排除的（比如 osd-scale-by-window 默认 true）。
/// 与其让上层在"看起来能播但行为不对"的状态下继续，不如当场失败并说清是哪一项。
[[nodiscard]] bool apply_mpv_options(mpv_handle* handle,
                                      const std::string& vo,
                                      const std::string& ao,
                                      const std::string& log_file,
                                      std::string& hwdec_out,
                                      std::string& error);

}  // namespace kivo::video::mpv
