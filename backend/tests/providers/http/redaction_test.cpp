#include "kivo/video/providers/http/http_range_provider.hpp"
#include "source_core/test_helpers.hpp"
#include <string>
using namespace kivo::video::source_core;
namespace ph=kivo::video::providers::http;
static ph::HttpTransportFn fx(uint64_t t){return[=](const ph::HttpTransportRequest& r)->ph::HttpTransportResponse{ph::HttpTransportResponse o;if(r.range_start.has_value()){o.status=206;o.headers["Content-Range"]="bytes 0-0/"+std::to_string(t);}return o;};}
int main(){ph::HttpRangeSessionStore s;auto o=ph::HttpRangeProvider::open(SourceOpenRequest{SourceUri::from_raw("http://example.com/video.mp4")},fx(100),s);CHECK_TRUE(o.is_success());auto ev=ph::HttpRangeProvider::get_evidence(o.session_if_success()->session_id,s);for(auto&i:ev.items)CHECK_TRUE(i.safe_description.find("http://example.com")==std::string::npos);return 0;}
