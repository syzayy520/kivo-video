#include "kivo/video/providers/http/http_url.hpp"
#include <cstring>
namespace kivo::video::providers::http {
static bool is_digit(char c) { return c>='0' && c<='9'; }
HttpUrlError parse_url(const std::string& raw, HttpUrl& out) {
    out.parsed = ParsedUrl{};
    if (raw.empty()) return HttpUrlError::empty;
    if (raw.find('\0') != std::string::npos) return HttpUrlError::has_nul;
    auto sep = raw.find("://");
    if (sep == std::string::npos) return HttpUrlError::no_scheme;
    out.parsed.scheme = raw.substr(0, sep);
    for (auto& c:out.parsed.scheme) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    if (out.parsed.scheme!="http" && out.parsed.scheme!="https") return HttpUrlError::invalid_scheme;
    auto host_start = sep + 3;
    auto at_pos = raw.find('@', host_start);
    if (at_pos != std::string::npos && at_pos < (raw.find('/', host_start))) return HttpUrlError::has_userinfo;
    auto slash = raw.find('/', host_start);
    auto host_part = (slash != std::string::npos) ? raw.substr(host_start, slash - host_start) : raw.substr(host_start);
    auto colon = host_part.rfind(':');
    if (colon != std::string::npos && colon > (host_part.find(']'))) {
        auto port_str = host_part.substr(colon+1);
        for (auto c:port_str) if (!is_digit(c)) return HttpUrlError::invalid_port;
        auto port_val = std::strtoull(port_str.c_str(), nullptr, 10);
        if (port_val == 0 || port_val > 65535) return HttpUrlError::invalid_port;
        out.parsed.port = static_cast<std::uint16_t>(port_val);
        out.parsed.has_port = true;
        host_part = host_part.substr(0, colon);
    }
    out.parsed.host = host_part;
    if (out.parsed.host.empty()) return HttpUrlError::missing_host;
    if (!out.parsed.has_port) out.parsed.port = (out.parsed.scheme=="https") ? 443 : 80;
    auto frag = raw.find('#', host_start);
    if (frag != std::string::npos) return HttpUrlError::has_fragment;
    out.parsed.path = (slash != std::string::npos) ? raw.substr(slash) : "/";
    out.parsed.valid = true;
    return HttpUrlError::none;
}
}  // namespace kivo::video::providers::http
