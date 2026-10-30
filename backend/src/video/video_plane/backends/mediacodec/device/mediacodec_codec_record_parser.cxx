#include "video/video_plane/backends/mediacodec/device/mediacodec_codec_record_parser.hpp"

#include <charconv>
#include <utility>
#include <vector>

namespace kivo::video::mediacodec {
namespace {

constexpr char kFieldSeparator = '\t';
constexpr std::size_t kExpectedFieldCount = 9;

[[nodiscard]] std::vector<std::string_view> split_fields(std::string_view raw) {
    std::vector<std::string_view> fields;
    std::size_t begin = 0;
    while (true) {
        const std::size_t pos = raw.find(kFieldSeparator, begin);
        if (pos == std::string_view::npos) {
            fields.push_back(raw.substr(begin));
            break;
        }
        fields.push_back(raw.substr(begin, pos - begin));
        begin = pos + 1;
    }
    return fields;
}

[[nodiscard]] bool parse_int32(std::string_view text, std::int32_t& out) {
    if (text.empty()) {
        return false;
    }
    std::int32_t value = 0;
    const char* first = text.data();
    const char* last = text.data() + text.size();
    const auto result = std::from_chars(first, last, value);
    if (result.ec != std::errc{} || result.ptr != last) {
        return false;
    }
    out = value;
    return true;
}

}  // namespace

bool parse_codec_record(std::string_view raw, CodecInventoryRecord& out) {
    const std::vector<std::string_view> fields = split_fields(raw);
    if (fields.size() != kExpectedFieldCount) {
        return false;
    }
    if (fields[0].empty() || fields[1].empty()) {
        return false;
    }

    CodecInventoryRecord parsed;
    parsed.name = std::string(fields[0]);
    parsed.mime = std::string(fields[1]);
    parsed.is_encoder = fields[2] == "1";

    if (!parse_int32(fields[3], parsed.profile) ||
        !parse_int32(fields[4], parsed.level) ||
        !parse_int32(fields[5], parsed.max_width) ||
        !parse_int32(fields[6], parsed.max_height) ||
        !parse_int32(fields[7], parsed.is_hardware) ||
        !parse_int32(fields[8], parsed.is_software_only)) {
        return false;
    }

    out = std::move(parsed);
    return true;
}

}  // namespace kivo::video::mediacodec
