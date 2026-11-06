#include "video/subtitle_plane/render/libass_renderer.hpp"

#include <algorithm>
#include <cstring>

namespace kivo::video::subtitle_plane::render {

LibassRenderer::LibassRenderer() {
    library_ = ass_library_init();
    if (library_ != nullptr) {
        // 无系统字体提供者（Android 无 fontconfig；设计稿 D4）：
        // 全部字体走 add_font_file 内存注入。
        ass_set_fonts(library_, nullptr, nullptr, ASS_FONTPROVIDER_NONE, nullptr, 1);
        renderer_ = ass_renderer_init(library_);
        if (renderer_ != nullptr) {
            ass_set_frame_size(renderer_, 1920, 1080);  // 默认画布；真机按视频尺寸重设
        }
    }
}

LibassRenderer::~LibassRenderer() {
    if (track_ != nullptr) {
        ass_free_track(track_);
        track_ = nullptr;
    }
    if (renderer_ != nullptr) {
        ass_renderer_done(renderer_);
        renderer_ = nullptr;
    }
    if (library_ != nullptr) {
        ass_library_done(library_);
        library_ = nullptr;
    }
}

bool LibassRenderer::add_font_file(const std::string& path, std::string& error) {
    if (library_ == nullptr) {
        error = "library not initialized";
        return false;
    }
    FILE* fp = std::fopen(path.c_str(), "rb");
    if (fp == nullptr) {
        error = "cannot open font file: " + path;
        return false;
    }
    std::fseek(fp, 0, SEEK_END);
    const long size = std::ftell(fp);
    std::fseek(fp, 0, SEEK_SET);
    if (size <= 0) {
        std::fclose(fp);
        error = "font file is empty: " + path;
        return false;
    }
    // ass_add_font 不拷贝数据——字节由本类的 font_storage_ 持有。
    font_storage_.emplace_back(static_cast<std::size_t>(size));
    const auto read = std::fread(font_storage_.back().data(), 1,
                                 static_cast<std::size_t>(size), fp);
    std::fclose(fp);
    if (read != static_cast<std::size_t>(size)) {
        font_storage_.pop_back();
        error = "short read on font file: " + path;
        return false;
    }
    // 名字参数只是提示（libass 用字体内部名匹配为主）。
    ass_add_font(library_, "injected", font_storage_.back().data(),
                 static_cast<int>(font_storage_.back().size()));
    return true;
}

int LibassRenderer::load(const std::string& path, std::string& error) {
    if (library_ == nullptr || renderer_ == nullptr) {
        error = "library not initialized";
        return -1;
    }
    if (track_ != nullptr) {
        ass_free_track(track_);
        track_ = nullptr;
    }
    // 按扩展名分派：.ass 走全语义（样式/定位），其余当 srt。
    const std::string lower = [&path] {
        std::string out;
        for (char c : path) {
            out += static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        }
        return out;
    }();
    const bool is_ass = lower.size() >= 4 && lower.compare(lower.size() - 4, 4, ".ass") == 0;
    track_format_ = is_ass ? "ass" : "srt";
    track_ = ass_read_file(library_, const_cast<char*>(path.c_str()),
                           const_cast<char*>(is_ass ? "ass" : "srt"));
    if (track_ == nullptr) {
        error = "ass_read_file failed for " + path;
        return -1;
    }
    return track_->n_events;
}

OverlayBitmap LibassRenderer::render(int now_ms, bool* detect_change) {
    OverlayBitmap out;
    if (track_ == nullptr || renderer_ == nullptr) {
        return out;
    }
    int change = 0;
    ASS_Image* images = ass_render_frame(renderer_, track_, now_ms, &change);
    if (detect_change != nullptr) {
        *detect_change = change != 0;
    }
    if (images == nullptr) {
        return out;  // 该时刻无字幕：空位图
    }
    // 两遍：先从 ASS_Image 链求包围盒（链上各块尺寸/位置任意），
    // 再分配目标位图、逐块混合。
    int min_x = 1 << 30;
    int min_y = 1 << 30;
    int max_x = 0;
    int max_y = 0;
    for (const ASS_Image* img = images; img != nullptr; img = img->next) {
        if (img->w <= 0 || img->h <= 0) {
            continue;
        }
        min_x = std::min(min_x, img->dst_x);
        min_y = std::min(min_y, img->dst_y);
        max_x = std::max(max_x, img->dst_x + img->w);
        max_y = std::max(max_y, img->dst_y + img->h);
    }
    if (max_x <= min_x || max_y <= min_y) {
        return out;
    }
    out.width = max_x - min_x;
    out.height = max_y - min_y;
    out.rgba.assign(static_cast<std::size_t>(out.width) * out.height * 4, 0);
    const std::int64_t stride =
        static_cast<std::int64_t>(out.width) * 4;
    for (const ASS_Image* img = images; img != nullptr; img = img->next) {
        if (img->w <= 0 || img->h <= 0) {
            continue;
        }
        AssImageSpan span;
        span.bitmap = img->bitmap;
        span.width = img->w;
        span.height = img->h;
        span.stride = img->stride;
        // libass 颜色：RGBA，alpha 在最高字节。
        span.r = static_cast<std::uint8_t>((img->color >> 24) & 0xFF);
        span.g = static_cast<std::uint8_t>((img->color >> 16) & 0xFF);
        span.b = static_cast<std::uint8_t>((img->color >> 8) & 0xFF);
        span.a = static_cast<std::uint8_t>(img->color & 0xFF);
        span.dst_x = img->dst_x - min_x;
        span.dst_y = img->dst_y - min_y;
        blend_ass_image(out.rgba.data(), out.width, out.height, stride, span);
    }
    return out;
}

int LibassRenderer::event_count() const {
    return track_ != nullptr ? track_->n_events : 0;
}

std::string LibassRenderer::track_format() const {
    return track_format_;
}

}  // namespace kivo::video::subtitle_plane::render
