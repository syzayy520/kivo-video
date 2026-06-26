#include "kivo/video/providers/http/http_range_provider.hpp"
#include "source_core/test_helpers.hpp"
using namespace kivo::video::source_core;
namespace ph=kivo::video::providers::http;
static ph::HttpTransportFn fx(uint64_t t){return[=](const ph::HttpTransportRequest& r)->ph::HttpTransportResponse{ph::HttpTransportResponse o;if(r.range_start.has_value()&&*r.range_start<t&&r.range_end.has_value()){auto e=std::min(*r.range_end,t-1);o.status=206;o.headers["Content-Range"]="bytes "+std::to_string(*r.range_start)+"-"+std::to_string(e)+"/"+std::to_string(t);}return o;};}
int main(){
    ph::HttpRangeSessionStore s;auto o=ph::HttpRangeProvider::open(SourceOpenRequest{SourceUri::from_raw("http://x.com/f")},fx(6),s);
    CHECK_TRUE(o.is_success());auto id=o.session_if_success()->session_id;
    SourceSeekRequest sr{id,2,SeekOrigin::begin};
    auto rr=ph::HttpRangeProvider::seek(sr,s);
    CHECK_TRUE(rr.is_success());CHECK_HAS_VALUE(rr.new_position);CHECK_EQ(*rr.new_position,2ULL);
    SourceSeekRequest br{id,100,SeekOrigin::begin};
    CHECK_TRUE(!ph::HttpRangeProvider::seek(br,s).is_success());
    return 0;
}
