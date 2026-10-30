#include "video/video_plane/backends/mediacodec/device/mediacodec_json_string_escaper.hpp"

#include <cstdio>

namespace kivo::video::mediacodec {

std::string escape_json_string(std::string_view input) {
    std::string out;
    out.reserve(input.size() + 8);

    for (const char ch : input) {
        switch (ch) {
            case '"':
                out += "\\\"";
                break;
            case '\\':
                out += "\\\\";
                break;
            case '\n':
                out += "\\n";
                break;
            case '\r':
                out += "\\r";
                break;
            case '\t':
                out += "\\t";
                break;
            default:
                if (static_cast<unsigned char>(ch) < 0x20) {
                    char escaped[7] = {};
                    std::snprintf(escaped, sizeof(escaped), "\\u%04x",
                                  static_cast<unsigned>(static_cast<unsigned char>(ch)));
                    out += escaped;
                } else {
                    out += ch;
                }
                break;
        }
    }
    return out;
}

}  // namespace kivo::video::mediacodec
