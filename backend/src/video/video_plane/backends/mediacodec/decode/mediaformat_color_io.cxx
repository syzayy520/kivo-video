#include "video/video_plane/backends/mediacodec/decode/mediaformat_color_io.hpp"

#include "video/video_plane/backends/mediacodec/decode/color_key_names.hpp"

#include <cstddef>

namespace kivo::video::mediacodec {
namespace {

/// `AMediaFormat_getBuffer` 是 API 28 才导出的符号，而 minSdk = 26。
/// 这里直接调用，与 `mediacodec_format_guard.cxx` 里判断 `csd-0` 存在性的写法一致 ——
/// 工程在这条路径上只支持 API 28+ 的设备（K40 为 Android 13）。
/// 若将来要下探到 26，这里和 format_guard 必须一起改成 dlsym。
[[nodiscard]] bool has_buffer(AMediaFormat* format, const char* key, std::int32_t* size_out) {
    void* data = nullptr;
    std::size_t size = 0;
    if (!AMediaFormat_getBuffer(format, key, &data, &size) || data == nullptr || size == 0) {
        return false;
    }
    if (size_out != nullptr) {
        *size_out = static_cast<std::int32_t>(size);
    }
    return true;
}

void copy_int32_if_present(AMediaFormat* from, AMediaFormat* to, const char* key) {
    std::int32_t value = 0;
    if (AMediaFormat_getInt32(from, key, &value)) {
        AMediaFormat_setInt32(to, key, value);
    }
}

void copy_buffer_if_present(AMediaFormat* from, AMediaFormat* to, const char* key) {
    void* data = nullptr;
    std::size_t size = 0;
    if (AMediaFormat_getBuffer(from, key, &data, &size) && data != nullptr && size > 0) {
        AMediaFormat_setBuffer(to, key, data, size);
    }
}

}  // namespace

SourceColorTags read_source_color_tags(const AMediaFormat* track_format) {
    SourceColorTags tags;
    if (track_format == nullptr) {
        return tags;
    }
    auto* format = const_cast<AMediaFormat*>(track_format);

    std::int32_t value = -1;
    if (AMediaFormat_getInt32(format, color_key::kKeyColorStandard, &value)) {
        tags.standard = value;
    }
    if (AMediaFormat_getInt32(format, color_key::kKeyColorTransfer, &value)) {
        tags.transfer = value;
    }
    if (AMediaFormat_getInt32(format, color_key::kKeyColorRange, &value)) {
        tags.range = value;
    }
    // 静态元数据在容器里是 `mdcv`/`clli` 盒；SEI 里有、盒里没有是常态。
    // 读不到就保持 -1，**不许**据此断言"这份片源没有 HDR 静态元数据"。
    std::int32_t size = 0;
    if (has_buffer(format, color_key::kKeyHdrStaticInfo, &size)) {
        tags.static_info_bytes = size;
    }
    return tags;
}

AMediaFormat* make_configure_format(const AMediaFormat* track_format,
                                   const SourceColorTags& tags) {
    if (track_format == nullptr) {
        return nullptr;
    }
    auto* from = const_cast<AMediaFormat*>(track_format);
    AMediaFormat* out = AMediaFormat_new();
    if (out == nullptr) {
        return nullptr;
    }

    // 关键几项：mime / 几何 / 参数集 / 输入缓冲上界。
    // csd-0 必须抄 —— AVC/HEVC 没有 SPS/PPS 连 configure 都过不去（E1 探针已验证）。
    const char* mime = nullptr;
    if (AMediaFormat_getString(from, AMEDIAFORMAT_KEY_MIME, &mime) && mime != nullptr) {
        AMediaFormat_setString(out, AMEDIAFORMAT_KEY_MIME, mime);
    }
    copy_int32_if_present(from, out, AMEDIAFORMAT_KEY_WIDTH);
    copy_int32_if_present(from, out, AMEDIAFORMAT_KEY_HEIGHT);
    copy_int32_if_present(from, out, AMEDIAFORMAT_KEY_MAX_INPUT_SIZE);
    copy_buffer_if_present(from, out, "csd-0");
    copy_buffer_if_present(from, out, "csd-1");

    // 颜色键：**源有的才写**。这就是"下发"这个动作本身 ——
    // 从"平台碰巧从轨道格式里读到了"变成"我们明确写进去了"。
    if (tags.standard >= 0) {
        AMediaFormat_setInt32(out, color_key::kKeyColorStandard, tags.standard);
    }
    if (tags.transfer >= 0) {
        AMediaFormat_setInt32(out, color_key::kKeyColorTransfer, tags.transfer);
    }
    if (tags.range >= 0) {
        AMediaFormat_setInt32(out, color_key::kKeyColorRange, tags.range);
    }
    if (tags.static_info_bytes > 0) {
        copy_buffer_if_present(from, out, color_key::kKeyHdrStaticInfo);
    }
    return out;
}

AMediaFormat* make_configure_format_with_transfer_request(const AMediaFormat* track_format,
                                                          const SourceColorTags& tags,
                                                          std::int32_t requested_transfer) {
    AMediaFormat* out = make_configure_format(track_format, tags);
    if (out == nullptr) {
        return nullptr;
    }
    AMediaFormat_setInt32(out, color_key::kKeyColorTransferRequest, requested_transfer);
    return out;
}

}  // namespace kivo::video::mediacodec
