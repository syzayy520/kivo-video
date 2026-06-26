#include "kivo/video/providers/http/http_range_provider.hpp"
#include "source_core/test_helpers.hpp"
using namespace kivo::video::source_core; namespace ph=kivo::video::providers::http;
static ph::HttpTransportFn fx_ext(){return[](const ph::HttpTransportRequest& r)->ph::HttpTransportResponse{ph::HttpTransportResponse o;if(r.range_start==0&&r.range_end==0){o.status=206;o.headers["Content-Range"]="bytes 0-0/6";o.body.push_back('a');}else if(r.range_start==0&&r.range_end==1){o.status=206;o.headers["Content-Range"]="bytes 0-1/6";o.body.push_back('a');o.body.push_back('b');}else if(r.range_start.has_value()){o.status=206;auto s=r.range_start.value();auto e=std::min(r.range_end.value(),5ULL);o.headers["Content-Range"]="bytes "+std::to_string(s)+"-"+std::to_string(e)+"/6";}return o;};}
int main(){ph::HttpRangeSessionStore s;auto o=ph::HttpRangeProvider::open(SourceOpenRequest{SourceUri::from_raw("http://x.com/f")},fx_ext(),s);CHECK_TRUE(o.is_success());return 0;}
