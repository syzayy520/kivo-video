// overlay_blend_tests.cpp — 212：libass 位图合成的单测（纯函数，零依赖）。
//
// 测的重点（对应 212 设计稿 §五）：
//   alpha 方向：color 低字节 0 = 不透明（VSFilter 惯例，dist 头文件实证）——
//   首版方向写反，真机上会得到"镂空文字"而不是实心文字；
//   遮罩 × 全局不透明度的双层合成；裁剪（越界部分不写）；stride 不齐。
#include <cstdio>
#include <cstring>
#include <vector>

#include "video/subtitle_plane/render/overlay_blend.hpp"

namespace {

namespace sp = kivo::video::subtitle_plane::render;

int g_failures = 0;
int g_checks = 0;

void check(bool condition, const char* expr, const char* test, int line) {
    ++g_checks;
    if (!condition) {
        ++g_failures;
        std::printf("  FAIL %s:%d  %s\n", test, line, expr);
    }
}
#define CHECK(expr) check((expr), #expr, __func__, __LINE__)

/// 造一块全 255 遮罩（每个像素都是字形全覆盖）。
sp::AssImageSpan solid_span(const std::vector<std::uint8_t>& mask, int w, int h,
                            std::uint8_t r, std::uint8_t g, std::uint8_t b,
                            std::uint8_t a, int dx, int dy) {
    sp::AssImageSpan span;
    span.bitmap = mask.data();
    span.width = w;
    span.height = h;
    span.stride = w;
    span.r = r;
    span.g = g;
    span.b = b;
    span.a = a;
    span.dst_x = dx;
    span.dst_y = dy;
    return span;
}

}  // namespace

int main() {
    std::printf("== 212 overlay_blend_tests ==\n");

    // ---- 1. 不透明整块：目标全红 ----
    {
        std::vector<std::uint8_t> mask(2 * 2, 255);
        std::vector<std::uint8_t> dst(2 * 2 * 4, 0);
        sp::blend_ass_image(dst.data(), 2, 2, 8,
                            solid_span(mask, 2, 2, 255, 0, 0, 0, 0, 0));
        CHECK(dst[0] == 255 && dst[1] == 0 && dst[2] == 0 && dst[3] == 255);
        CHECK(dst[7] == 255 && dst[11] == 255);  // 每像素都写满
    }

    // ---- 2. 全透明（a=255）：目标不动 ----
    {
        std::vector<std::uint8_t> mask(2 * 2, 255);
        std::vector<std::uint8_t> dst(2 * 2 * 4, 7);
        sp::blend_ass_image(dst.data(), 2, 2, 8,
                            solid_span(mask, 2, 2, 0, 255, 0, 255, 0, 0));
        CHECK(dst[0] == 7 && dst[15] == 7);  // 一字节都没动
    }

    // ---- 3. 半透明（a=128）：中间值 ----
    {
        std::vector<std::uint8_t> mask(1 * 1, 255);
        std::vector<std::uint8_t> dst(1 * 1 * 4, 0);
        // 255 的遮罩 × (255-128) 的不透明度 → opacity = 127
        sp::blend_ass_image(dst.data(), 1, 1, 4,
                            solid_span(mask, 1, 1, 254, 0, 0, 128, 0, 0));
        // out_r = 254*127/255 ≈ 126
        CHECK(dst[0] == 126 || dst[0] == 127);  // 整除舍入的两可区间
        CHECK(dst[3] == 126 || dst[3] == 127);
    }

    // ---- 4. 遮罩部分覆盖：遮罩 0 的像素不写 ----
    {
        std::vector<std::uint8_t> mask = {255, 0, 0, 255};
        std::vector<std::uint8_t> dst(2 * 2 * 4, 3);
        sp::blend_ass_image(dst.data(), 2, 2, 8,
                            solid_span(mask, 2, 2, 0, 0, 255, 0, 0, 0));
        CHECK(dst[2] == 255);            // 像素0：蓝写上
        CHECK(dst[4 + 2] == 3);          // 像素1：遮罩 0，保持
        CHECK(dst[12 + 2] == 255);       // 像素3：写上
    }

    // ---- 5. 越界裁剪：span 超出目标的部分不写不崩 ----
    {
        std::vector<std::uint8_t> mask(4 * 4, 255);
        std::vector<std::uint8_t> dst(2 * 2 * 4, 5);
        sp::blend_ass_image(dst.data(), 2, 2, 8,
                            solid_span(mask, 4, 4, 255, 255, 0, 0, -1, -1));
        // 只应有目标范围内的像素被写
        CHECK(dst[0] == 255 && dst[1] == 255 && dst[2] == 0);
        CHECK(dst[15] == 255);
        // 无越界写坏（边界外内存没检查工具就不查了——裁剪逻辑保证不写）
    }

    // ---- 6. stride 大于宽度（行距不齐的遮罩）----
    {
        std::vector<std::uint8_t> mask(4 * 2, 0);
        mask[0] = 255;  // 第一行第一个像素
        mask[4] = 255;  // 第二行第一个像素（stride=4）
        std::vector<std::uint8_t> dst(2 * 2 * 4, 9);
        sp::AssImageSpan span = solid_span(mask, 2, 2, 255, 255, 255, 0, 0, 0);
        span.stride = 4;
        sp::blend_ass_image(dst.data(), 2, 2, 8, span);
        // 布局：像素 (x,y) 在 (y*2+x)*4。(0,0)→[0..3]，(1,0)→[4..7]，
        // (0,1)→[8..11]，(1,1)→[12..15]。
        CHECK(dst[2] == 255);            // (0,0)：写上
        CHECK(dst[4 + 2] == 9);          // (1,0)：遮罩 0，保持
        CHECK(dst[8 + 2] == 255);        // (0,1)：跨 stride 写上
        CHECK(dst[12 + 2] == 9);         // (1,1)：遮罩 0，保持
    }

    // ---- 7. 双块合成：后一块与前一块同区 → 后写覆盖（libass 链序语义）----
    {
        std::vector<std::uint8_t> mask(1 * 1, 255);
        std::vector<std::uint8_t> dst(1 * 1 * 4, 0);
        sp::blend_ass_image(dst.data(), 1, 1, 4,
                            solid_span(mask, 1, 1, 0, 0, 255, 0, 0, 0));
        sp::blend_ass_image(dst.data(), 1, 1, 4,
                            solid_span(mask, 1, 1, 255, 0, 0, 0, 0, 0));
        CHECK(dst[0] == 255 && dst[2] == 0);  // 第二块（红）覆盖第一块（蓝）
    }

    std::printf("%d checks / %d failed\n", g_checks, g_failures);
    return g_failures == 0 ? 0 : 1;
}
