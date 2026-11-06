#pragma once

#include <cstdint>
#include <string>
#include <vector>

extern "C" {
#include <ass/ass.h>
}

namespace kivo::video::subtitle_plane::render {

/// 渲染产物：一块 RGBA 位图（预乘前直出，交给叠加层）。
struct OverlayBitmap {
    int width = 0;
    int height = 0;
    std::vector<std::uint8_t> rgba;  ///< 4 字节/像素；空 = 该时刻无字幕
};

/// libass 桥（212）：srt/ass 文件 → 指定时刻的 RGBA 位图。
///
/// 单一职责：libass C API 的生命周期与调用序列。字体走 `add_font_file`
/// 内存注入（Android 无 fontconfig；设计稿 D4）。
///
/// libass 全状态（library/renderer/track）**只在创建线程上使用**——
/// 211 的教训（FFmpeg 全生命周期单线程持有）同款纪律。
class LibassRenderer {
public:
    LibassRenderer();
    ~LibassRenderer();
    LibassRenderer(const LibassRenderer&) = delete;
    LibassRenderer& operator=(const LibassRenderer&) = delete;

    /// 注入字体文件（ttf/ttc/otf 原始字节整体读入）。可多次调用。
    [[nodiscard]] bool add_font_file(const std::string& path, std::string& error);

    /// 加载字幕文件（按扩展名分派：.ass → ASS 全语义；其余 → 当作 srt）。
    /// 返回事件数（0 = 没解析出任何条目，调用方据此给"无字幕"证据）。
    [[nodiscard]] int load(const std::string& path, std::string& error);

    /// 渲染 `now_ms` 时刻。无字幕/超出所有条目时返回空位图。
    [[nodiscard]] OverlayBitmap render(int now_ms, bool* detect_change);

    /// 已加载的事件数（AC1 的"文本事件数与文件一致"判据）。
    [[nodiscard]] int event_count() const;
    [[nodiscard]] std::string track_format() const;  ///< "ass" / "srt"

private:
    ASS_Library* library_ = nullptr;
    ASS_Renderer* renderer_ = nullptr;
    ASS_Track* track_ = nullptr;
    std::string default_font_path_;  ///< 首个成功注入的字体（ass_set_fonts 的回退字体）
    std::vector<std::vector<std::uint8_t>> font_storage_;  // ass_add_font 不拷贝，字节归本类
    std::string track_format_;
};

}  // namespace kivo::video::subtitle_plane::render
