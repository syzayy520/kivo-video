#include "kivo/video/providers/webdav/webdav_bridge.hpp"
#include "../../providers/http/test_fixture.hpp"
#include "source_core/test_helpers.hpp"
using namespace kivo::video::source_core; using namespace kivo::video::source_core::test;
namespace wd=kivo::video::providers::webdav; namespace ph=kivo::video::providers::http;
// PROPFIND returns 207 Multi-Status → probe_resource_type reports file_like
static ph::HttpTransportFn fx_collection(){return[](const ph::HttpTransportRequest& r)->ph::HttpTransportResponse{ph::HttpTransportResponse o;if(r.method=="PROPFIND"){o.status=207;o.headers["Content-Type"]="application/xml; charset=utf-8";}else if(r.range_start.has_value()){o.status=206;o.headers["Content-Range"]="bytes 0-0/6";o.body.push_back(static_cast<std::uint8_t>('a'));}return o;};}
// PROPFIND returns 501 Not Implemented → propfind_failed
static ph::HttpTransportFn fx_no_propfind(){return[](const ph::HttpTransportRequest& r)->ph::HttpTransportResponse{ph::HttpTransportResponse o;if(r.method=="PROPFIND"){o.status=501;}return o;};}
// No PROPFIND response at all → resource_type_unknown
static ph::HttpTransportFn fx_no_resp(){return[](const ph::HttpTransportRequest&)->ph::HttpTransportResponse{return {};};}
int main(){
    // 207 Multi-Status → file_like (PROPFIND supported, resource readable)
    auto info=wd::probe_resource_type("http://x.com/c",fx_collection());
    CHECK_EQ(info.probe,wd::WebDavProbeResult::file_like);
    // 501 Not Implemented → propfind_failed
    auto info2=wd::probe_resource_type("http://x.com/d",fx_no_propfind());
    CHECK_EQ(info2.probe,wd::WebDavProbeResult::propfind_failed);
    // No response → resource_type_unknown  
    auto info3=wd::probe_resource_type("http://x.com/e",fx_no_resp());
    CHECK_EQ(info3.probe,wd::WebDavProbeResult::resource_type_unknown);
    // Collection-like resource: 207 but open should still succeed (bridged to HTTP range)
    ph::HttpRangeSessionStore s;
    auto r=wd::WebDavRangeBridge::open(SourceOpenRequest{SourceUri::from_raw("http://x.com/c")},fx_collection(),s);
    CHECK_TRUE(r.is_success());
    return 0;
}
