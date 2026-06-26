#include "kivo/video/providers/http/http_range_provider.hpp"
#include "source_core/test_helpers.hpp"
using namespace kivo::video::source_core; namespace ph=kivo::video::providers::http;
static ph::HttpTransportFn fx_loop(){return[=,c=std::make_shared<int>(0)](const ph::HttpTransportRequest&)mutable->ph::HttpTransportResponse{ph::HttpTransportResponse o;(*c)++;if(*c>5){o.status=206;o.headers["Content-Range"]="bytes 0-0/6";o.body.push_back('a');}else{o.status=301;o.headers["Location"]="http://loop.example.com/"+std::to_string(*c);}return o;};}
int main(){ph::HttpRangeSessionStore s;auto r=ph::HttpRangeProvider::open(SourceOpenRequest{SourceUri::from_raw("http://loop.example.com/0")},fx_loop(),s);(void)r;return 0;}
