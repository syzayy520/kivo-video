#include "kivo/video/providers/http/http_range_provider.hpp"
#include "source_core/test_helpers.hpp"
using namespace kivo::video::source_core;
using namespace kivo::video::source_core::test;
namespace ph = kivo::video::providers::http;

static ph::HttpTransportFn make_range_fixture(std::uint64_t total_size, const std::string& body="abcdef") {
    return [=](const ph::HttpTransportRequest& req)->ph::HttpTransportResponse {
        ph::HttpTransportResponse r;
        if (req.method=="GET" && req.range_start.has_value()) {
            auto s=req.range_start.value(); auto e=req.range_end.value();
            if (s==0 && e==0) { r.status=206; r.headers["Content-Range"]="bytes 0-0/"+std::to_string(total_size); r.body.assign(body.begin(),body.begin()+std::min(std::size_t(1),body.size())); }
            else if (s<total_size) {
                auto len=std::min(e-s+1,total_size-s);
                r.status=206; r.headers["Content-Range"]="bytes "+std::to_string(s)+"-"+std::to_string(s+len-1)+"/"+std::to_string(total_size);
                if (s<static_cast<std::uint64_t>(body.size())) { auto end=std::min(static_cast<std::uint64_t>(body.size()),s+len); r.body.assign(body.begin()+s,body.begin()+end); }
            }
        }
        return r;
    };
}

int main() {
    auto fx=make_range_fixture(6,"abcdef");
    ph::HttpRangeSessionStore store;
    auto rr=ph::HttpRangeProvider::open(SourceOpenRequest{SourceUri::from_raw("http://example.com/file.mp4")},fx,store);
    CHECK_TRUE(rr.is_success());
    CHECK_EQ(rr.session_if_success()->session_state,SourceSessionState::open);
    CHECK_EQ(rr.session_if_success()->provider_kind,ProviderKind::http_file);
    return 0;
}
