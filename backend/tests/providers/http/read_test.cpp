#include "kivo/video/providers/http/http_range_provider.hpp"
#include "source_core/test_helpers.hpp"
using namespace kivo::video::source_core;
namespace ph = kivo::video::providers::http;
static ph::HttpTransportFn make_fixture(std::uint64_t total,const std::string& body) {
    return [=](const ph::HttpTransportRequest& req)->ph::HttpTransportResponse {
        ph::HttpTransportResponse r;
        if (req.range_start.has_value()) {
            auto s=req.range_start.value(); auto e=req.range_end.value();
            if (s<total){ auto len=std::min(e-s+1,total-s); r.status=206; r.headers["Content-Range"]="bytes "+std::to_string(s)+"-"+std::to_string(s+len-1)+"/"+std::to_string(total); if(s<static_cast<std::uint64_t>(body.size())){auto en=std::min(static_cast<std::uint64_t>(body.size()),s+len); r.body.assign(body.begin()+s,body.begin()+en);} }
        }
        return r;
    };
}
int main() {
    auto fx=make_fixture(6,"abcdef");
    ph::HttpRangeSessionStore store;
    auto orr=ph::HttpRangeProvider::open(SourceOpenRequest{SourceUri::from_raw("http://example.com/f.mp4")},fx,store);
    CHECK_TRUE(orr.is_success());
    auto sid=orr.session_if_success()->session_id;
    SourceReadRequest r1{sid,std::nullopt,3};
    auto rr1=ph::HttpRangeProvider::read(r1,fx,store);
    CHECK_TRUE(rr1.is_success());
    CHECK_EQ(rr1.bytes_read,3ULL);
    CHECK_EQ(rr1.bytes[0],(std::uint8_t)'a');
    SourceReadRequest r2{sid,4ULL,1};
    auto rr2=ph::HttpRangeProvider::read(r2,fx,store);
    CHECK_TRUE(rr2.is_success());
    CHECK_EQ(rr2.bytes[0],(std::uint8_t)'e');
    return 0;
}
