#pragma once
#include <cstdint>
#include <string>
namespace kivo::video::providers::http {
enum class ContentRangeError : std::uint8_t { none=0, not_bytes=1, invalid_format=2,
    overflow=3, negative=4, suffix_range=5, total_mismatch=6 };
struct ParsedContentRange { std::uint64_t start{0}; std::uint64_t end{0};
    std::uint64_t total{0}; bool has_total{false}; bool valid{false}; };
ContentRangeError parse_content_range(const std::string& header, ParsedContentRange& out);
}  // namespace kivo::video::providers::http
