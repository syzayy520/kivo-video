#include "kivo/video/providers/http/http_range_provider.hpp"
#include "source_core/test_helpers.hpp"
using namespace kivo::video::source_core;
namespace ph=kivo::video::providers::http;
static ph::HttpTransportFn fx(uint64_t t){return[=](const ph::HttpTransportRequest& r)->ph::HttpTransportResponse{ph::HttpTransportResponse o;if(r.range_start.has_value()){o.status=206;o.headers["Content-Range"]="bytes "+std::to_string(r.range_start.value())+"-0/"+std::to_string(t);}return o;};}
int main(){ph::HttpRangeSessionStore s;auto o=ph::HttpRangeProvider::open(SourceOpenRequest{SourceUri::from_raw("http://x.com/f")},fx(10),s);CHECK_TRUE(o.is_success());auto id=o.session_if_success()->session_id;CHECK_TRUE(ph::HttpRangeProvider::close(id,s).is_ok());CHECK_TRUE(ph::HttpRangeProvider::close(id,s).is_ok());CHECK_TRUE(!ph::HttpRangeProvider::read(SourceReadRequest{id,std::nullopt,1},fx(10),s).is_success());return 0;}
