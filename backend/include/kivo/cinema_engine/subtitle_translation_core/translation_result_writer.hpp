#pragma once

// ============================================================================
// 译文写出：把带时间戳的译文写成 SRT 文件文本。
//
// 落位路径: backend/include/kivo/cinema_engine/subtitle_translation_core/
//            translation_result_writer.hpp
//          backend/src/subtitle_translation_core/translation_result_writer.cpp
//
// 为什么需要它（仓库里原本没有）：
//   既有 subtitle_plane/render/srt_convert 只做 **SRT → ASS**（读进来渲染）。
//   翻译链路要的是反方向 —— 把译文**写成 .srt 落盘**，
//   再交给播放器的外挂字幕入口加载。所以这块是净新增，不是重复造轮子。
//
// 与播放器的接点极小：产物写进 cacheDir/subs/ 之后，
// `mpv.command("sub-add", "<path> select")` 即可启用 ——
// 与用户手动导入外挂字幕是同一条路，播放链路零改动。
//
// 零平台依赖：只用 std::string。
// ============================================================================

#include <string>
#include <vector>

#include "kivo/cinema_engine/subtitle_translation_core/subtitle_translation_types.hpp"

namespace kivo::cinema_engine {

/// 把带时间戳的译文序列写成 SRT 文本。
///
/// 格式口径：
///   - 序号行从 1 开始连续编号（**只给真正写出的行编号**，
///     翻译失败的行被跳过时序号不跳号，否则播放器可能因序号不连续而拒绝解析）
///   - 时间行 `HH:MM:SS,mmm --> HH:MM:SS,mmm`，毫秒三位补零
///   - 行尾 CRLF（SRT 规范；播放器与 mpv 都吃）
///   - 块之间空一行
///
/// 跳过规则：text 为空、或 end_time_ms <= start_time_ms 的行不写。
/// 「翻译失败」的行必须由调用方先从序列里剔除（或置空 text），
/// 这里不做成功/失败的判断 —— 那是翻译层的职责。
///
/// @param lines  带时间戳的译文，按播放顺序。
/// @return       SRT 文本（UTF-8）。没有任何可写行时返回空串。
std::string write_translated_srt(const std::vector<SubtitleTranslationTimedLine>& lines);

}  // namespace kivo::cinema_engine
