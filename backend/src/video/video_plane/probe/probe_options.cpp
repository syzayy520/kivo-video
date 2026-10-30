#include "video/video_plane/probe/probe_options.hpp"

#include <cstdlib>
#include <cstring>

namespace kivo::video::video_plane::probe {
namespace {

[[nodiscard]] auto parse_int(const char* text, std::int32_t& out) noexcept -> bool {
    if (text == nullptr || *text == '\0') {
        return false;
    }
    char* end = nullptr;
    const long value = std::strtol(text, &end, 10);
    if (end == text || (end != nullptr && *end != '\0') || value <= 0 || value > 1000000) {
        return false;
    }
    out = static_cast<std::int32_t>(value);
    return true;
}

}  // namespace

auto probe_usage() -> const char* {
    return
        "kivo_hw_decode_probe --input <file> [options]\n"
        "\n"
        "  --input <path>        media file to decode (required)\n"
        "  --frames <n>          stop after n decoded frames (default 300)\n"
        "  --compare-software    also measure a pure CPU decode for comparison\n"
        "  --evidence <path>     write a JSON evidence file\n"
        "  --help                show this text\n";
}

auto parse_probe_options(const int argc, char** argv, ProbeOptions& out, std::string& error)
    -> bool {
    for (int i = 1; i < argc; ++i) {
        const char* arg = argv[i];
        if (arg == nullptr) {
            continue;
        }
        const auto next_value = [&]() -> const char* {
            return (i + 1 < argc) ? argv[++i] : nullptr;
        };

        if (std::strcmp(arg, "--input") == 0) {
            const char* value = next_value();
            if (value == nullptr) {
                error = "--input requires a path";
                return false;
            }
            out.input_path = value;
        } else if (std::strcmp(arg, "--frames") == 0) {
            if (!parse_int(next_value(), out.max_frames)) {
                error = "--frames requires a positive integer";
                return false;
            }
        } else if (std::strcmp(arg, "--compare-software") == 0) {
            out.compare_software = true;
        } else if (std::strcmp(arg, "--evidence") == 0) {
            const char* value = next_value();
            if (value == nullptr) {
                error = "--evidence requires a path";
                return false;
            }
            out.evidence_path = value;
        } else if (std::strcmp(arg, "--help") == 0 || std::strcmp(arg, "-h") == 0) {
            error = "help requested";
            return false;
        } else {
            error = std::string("unknown argument: ") + arg;
            return false;
        }
    }

    if (out.input_path.empty()) {
        error = "--input is required";
        return false;
    }
    return true;
}

}  // namespace kivo::video::video_plane::probe
