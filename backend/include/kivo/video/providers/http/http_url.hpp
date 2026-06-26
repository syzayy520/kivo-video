#pragma once
#include <string>
#include <cstdint>
namespace kivo::video::providers::http {
enum class HttpUrlError : std::uint8_t { none=0, empty=1, no_scheme=2, invalid_scheme=3,
    invalid_host=4, missing_host=5, invalid_port=6, has_userinfo=7, has_fragment=8, has_nul=9 };
struct ParsedUrl { std::string scheme; std::string host; std::uint16_t port{0}; std::string path;
    bool has_port{false}; bool valid{false}; };
struct HttpUrl {
    ParsedUrl parsed;
    std::string safe_redacted() const { return parsed.scheme+"://"+parsed.host+"/<redacted>"; }
};
HttpUrlError parse_url(const std::string& raw, HttpUrl& out);
}  // namespace kivo::video::providers::http
