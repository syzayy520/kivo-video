#include "kivo/video/providers/webdav/webdav_bridge.hpp"
#include "../../providers/http/test_fixture.hpp"
#include "source_core/test_helpers.hpp"
using namespace kivo::video::source_core; using namespace kivo::video::source_core::test;
namespace wd=kivo::video::providers::webdav; namespace ph=kivo::video::providers::http;
static ph::HttpTransportFn fx_collection(){return [](const ph::HttpTransportRequest& r)->ph::HttpTransportResponse{ph::HttpTransportResponse o;if(r.method=="PROPFIND"){o.status=207;}return o;};}
static ph::HttpTransportFn fx_file(){return [](const ph::HttpTransportRequest& r)->ph::HttpTransportResponse{ph::HttpTransportResponse o;if(r.method=="PROPFIND"){o.status=405;}else if(r.range_start.has_value()){o.status=206;o.headers["Content-Range"]="bytes 0-0/6";}return o;};}
int main(){
    ph::HttpRangeSessionStore s;
    auto rc=wd::WebDavRangeBridge::open(SourceOpenRequest{SourceUri::from_raw("http://x.com/c")},fx_collection(),s);
    CHECK_TRUE(rc.is_failure()); // collection rejected
    auto rf=wd::WebDavRangeBridge::open(SourceOpenRequest{SourceUri::from_raw("http://x.com/f")},fx_file(),s);
    CHECK_TRUE(rf.is_success()); // file-like, bridge to HTTP
    return 0;
}
