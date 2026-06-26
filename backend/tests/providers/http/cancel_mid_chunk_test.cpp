#include "kivo/video/providers/http/http_range_provider.hpp"
#include "source_core/test_helpers.hpp"
using namespace kivo::video::source_core; namespace ph=kivo::video::providers::http;
static ph::HttpTransportFn fx_block(){return[=,c=std::make_shared<int>(0)](const ph::HttpTransportRequest& r)mutable->ph::HttpTransportResponse{(*c)++;if(*c==1&&r.range_start==0&&r.range_end==0){ph::HttpTransportResponse o;o.status=206;o.headers["Content-Range"]="bytes 0-0/6";o.body.push_back('a');return o;}if(r.range_start.has_value()){ph::HttpTransportResponse o;o.status=499;return o;}ph::HttpTransportResponse o;return o;};}
int main(){ph::HttpRangeSessionStore s;auto o=ph::HttpRangeProvider::open(SourceOpenRequest{SourceUri::from_raw("http://x.com/f")},fx_block(),s);CHECK_TRUE(o.is_success());auto id=o.session_if_success()->session_id;auto rr=ph::HttpRangeProvider::read(SourceReadRequest{id,std::nullopt,3},fx_block(),s);CHECK_TRUE(!rr.is_success());return 0;}
