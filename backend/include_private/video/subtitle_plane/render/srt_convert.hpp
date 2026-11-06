#pragma once

#include <string>

namespace kivo::video::subtitle_plane::render {

/// SRT → ASS 文本转换（212）。
///
/// **libass 不解析 SRT**（0.16 全库无 TRACK_TYPE_SRT——设计稿 D1 的
/// "解析交给 libass" 被实测推翻）：libass 只是 ASS 渲染器，srt→ass 的
/// 转换是播放器层的事（mpv 同款架构）。本函数就是那一层：纯函数、桌面可测。
///
/// 覆盖 SRT 的常见形态：`序号` 行（可缺）、`HH:MM:SS,mmm --> HH:MM:SS,mmm` 时间行、
/// 多行文本（逐行拼接为 ASS 的 `\N`）、`{\...}` 花括号内容原样保留（SRT 里很少见，
/// 有就当 ASS 覆盖标签透传）。生成最小 ASS 头（PlayResX/Y 1920×1080 + Default 样式）
/// + Dialogue 行。
[[nodiscard]] std::string srt_to_ass(const std::string& srt_text);

/// 时间解析：`HH:MM:SS,mmm` → 毫秒。解析失败返回 -1。
[[nodiscard]] long long parse_srt_time(const std::string& text);

}  // namespace kivo::video::subtitle_plane::render
