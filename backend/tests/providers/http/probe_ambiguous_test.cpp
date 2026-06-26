#include "kivo/video/providers/http/http_range_provider.hpp"
#include "source_core/test_helpers.hpp"
using namespace kivo::video::source_core; namespace ph=kivo::video::providers::http;
static ph::HttpTransportFn fx_amb(){return[](const ph::HttpTransportRequest& r)->ph::HttpTransportResponse{ph::HttpTransportResponse o;if(r.range_start==0&&r.range_end==0){o.status=206;o.headers["Content-Range"]="bytes 0-0/*";}return o;};}
int main(){ph::HttpRangeSessionStore s;auto o=ph::HttpRangeProvider::open(SourceOpenRequest{SourceUri::from_raw("http://x.com/f")},fx_amb(),s);CHECK_TRUE(o.is_success());return 0;}
