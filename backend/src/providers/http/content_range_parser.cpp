#include "kivo/video/providers/http/content_range_parser.hpp"
#include <charconv>
#include <cstring>
namespace kivo::video::providers::http {
static bool is_digit(char c){return c>='0'&&c<='9';}
ContentRangeError parse_content_range(const std::string& header, ParsedContentRange& out) {
    out = ParsedContentRange{};
    auto s = header.data(); auto n = header.size();
    if (n<6 || std::strncmp(s,"bytes ",6)!=0) return ContentRangeError::not_bytes;
    s+=6; n-=6;

    // bytes */total: parse the total after the slash
    if (n>0 && s[0]=='*') {
        auto slash = 1ULL; while (slash<n && s[slash]!='/') slash++;
        if (slash>=n) { out.valid=true; out.has_total=false; return ContentRangeError::none; }
        auto t_str = s+slash+1;
        auto t_len = n-slash-1;
        if (t_len>0 && t_str[0]=='*') { out.valid=true; out.has_total=false; return ContentRangeError::none; }
        std::uint64_t total=0;
        auto [ptr,ec] = std::from_chars(t_str, t_str+t_len, total);
        if (ec==std::errc{} && ptr==t_str+t_len){
            out.valid=true; out.has_total=true; out.total=total; return ContentRangeError::none;
        }
        out.valid=true; out.has_total=false; return ContentRangeError::none;
    }

    // bytes start-end/total or bytes start-end/*
    if (n>0 && s[0]=='-') return ContentRangeError::negative;
    auto dash=0ULL; while(dash<n && s[dash]!='-') dash++;
    if (dash>=n||dash==0) return ContentRangeError::invalid_format;

    std::uint64_t start=0;
    auto [p1,e1] = std::from_chars(s, s+dash, start);
    if (e1!=std::errc{}) return ContentRangeError::overflow;

    s+=dash+1; n-=(dash+1);
    auto slash_pos=0ULL; while(slash_pos<n && s[slash_pos]!='/') slash_pos++;
    if (slash_pos>=n) return ContentRangeError::invalid_format;

    std::uint64_t end=0;
    auto [p2,e2] = std::from_chars(s, s+slash_pos, end);
    if (e2!=std::errc{}) return ContentRangeError::overflow;
    if (end<start) return ContentRangeError::invalid_format;

    s+=slash_pos+1;
    if (*s=='*') { out.start=start; out.end=end; out.has_total=false; out.valid=true; return ContentRangeError::none; }

    std::uint64_t total=0;
    auto [p3,e3] = std::from_chars(s, s+(n-slash_pos-1), total);
    if (e3!=std::errc{}) return ContentRangeError::overflow;

    out.start=start; out.end=end; out.total=total; out.has_total=true; out.valid=true;
    return ContentRangeError::none;
}
}  // namespace kivo::video::providers::http
