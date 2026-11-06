#pragma once

#include <cstdint>
#include <cstring>

namespace kivo::video::subtitle_plane::render {

/// libass 单块 ASS_Image 的颜色：RGBA，**alpha 在最高字节**（0xFF=不透明）。
struct AssImageSpan {
    const std::uint8_t* bitmap;  ///< 单通道 alpha 遮罩（每像素 1 字节）
    int width;
    int height;
    int stride;                  ///< 位图行距（字节）
    std::uint8_t r;
    std::uint8_t g;
    std::uint8_t b;
    std::uint8_t a;              ///< 整块图像的统一 alpha
    int dst_x;
    int dst_y;
};

/// 把一块 libass alpha 遮罩按其颜色/alpha 混合进 RGBA 目标位图。
///
/// 纯函数、零依赖——**桌面可测**（混色公式是本单元唯一有逻辑的部分，
/// libass 桥本身只是 C API 转发）。
///
/// **alpha 方向（以 dist 的 ass.h/ass_parse.h 为准）**：`color` 是
/// 0xRRGGBBAA，**低字节 0 = 完全不透明、255 = 完全透明**（VSFilter 惯例，
/// `_a(c) = c & 0xFF` + `parse_color_tag` 的 bswap 实证）；位图是逐像素
/// 遮罩（255 = 该像素字形全覆盖）。合成不透明度 = 遮罩 × (255 − color_alpha)。
inline void blend_ass_image(std::uint8_t* dst, int dst_width, int dst_height,
                            std::int64_t dst_stride_bytes, const AssImageSpan& span) {
    const int x0 = span.dst_x < 0 ? 0 : span.dst_x;
    const int y0 = span.dst_y < 0 ? 0 : span.dst_y;
    const int x1 = span.dst_x + span.width > dst_width ? dst_width
                                                       : span.dst_x + span.width;
    const int y1 = span.dst_y + span.height > dst_height ? dst_height
                                                         : span.dst_y + span.height;
    const std::uint32_t global_opacity = 255 - span.a;  // 0..255，255=不透明
    if (global_opacity == 0) {
        return;  // 整块全透明：libass 语义下没有任何像素可见
    }
    for (int y = y0; y < y1; ++y) {
        const std::uint8_t* mask =
            span.bitmap + static_cast<std::int64_t>(y - span.dst_y) * span.stride +
            (x0 - span.dst_x);
        std::uint8_t* out = dst + static_cast<std::int64_t>(y) * dst_stride_bytes +
                            static_cast<std::int64_t>(x0) * 4;
        for (int x = x0; x < x1; ++x, ++mask, out += 4) {
            // 合成不透明度 = 像素遮罩 × 整块不透明度。
            const std::uint32_t opacity =
                static_cast<std::uint32_t>(*mask) * global_opacity / 255;
            if (opacity == 0) {
                continue;
            }
            if (opacity >= 255) {
                out[0] = span.r;
                out[1] = span.g;
                out[2] = span.b;
                out[3] = 255;
                continue;
            }
            out[0] = static_cast<std::uint8_t>(
                (span.r * opacity + out[0] * (255 - opacity)) / 255);
            out[1] = static_cast<std::uint8_t>(
                (span.g * opacity + out[1] * (255 - opacity)) / 255);
            out[2] = static_cast<std::uint8_t>(
                (span.b * opacity + out[2] * (255 - opacity)) / 255);
            out[3] = static_cast<std::uint8_t>(
                opacity + out[3] * (255 - opacity) / 255);
        }
    }
}

}  // namespace kivo::video::subtitle_plane::render
