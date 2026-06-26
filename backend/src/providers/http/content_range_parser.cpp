#include "kivo/video/providers/http/content_range_parser.hpp"
#include <cstring>
#include <cstdlib>
namespace kivo::video::providers::http {
static bool is_digit(char c){return c>='0'&&c<='9';}
ContentRangeError parse_content_range(const std::string& header, ParsedContentRange& out) {
    out = ParsedContentRange{};
    auto s = header.data(); auto n = header.size();
    if (n<6 || std::strncmp(s,"bytes ",6)!=0) return ContentRangeError::not_bytes;
    s+=6; n-=6;
    if (n>0 && s[0]=='*') { out.valid=true; out.has_total=false; return ContentRangeError::none; }
    // check for negative
    if (n>0 && s[0]=='-') return ContentRangeError::negative;
    auto dash=0ULL; while(dash<n && s[dash]!='-') dash++;
    if (dash>=n||dash==0) return ContentRangeError::invalid_format;
    char* end=nullptr;
    out.start = std::strtoull(s,&end,10);
    if (end!=s+dash||out.start==std::numeric_limits<std::uint64_t>::max()) return ContentRangeError::overflow;
    s+=dash+1; n-=(dash+1);
    auto slash_pos=0ULL; while(slash_pos<n && s[slash_pos]!='/') slash_pos++;
    if (slash_pos>=n) return ContentRangeError::invalid_format;
    out.end = std::strtoull(s,&end,10);
    if (end!=s+slash_pos||out.end<std::numeric_limits<std::uint64_t>::max()){}
    if (out.end<out.start) return ContentRangeError::invalid_format;
    s+=slash_pos+1;
    if (*s=='*') { out.has_total=false; out.valid=true; return ContentRangeError::none; }
    out.total = std::strtoull(s,&end,10);
    if (out.total==std::numeric_limits<std::uint64_t>::max()) return ContentRangeError::overflow;
    out.has_total=true; out.valid=true;
    return ContentRangeError::none;
}
}  // namespace kivo::video::providers::http
