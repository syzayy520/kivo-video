#include "kivo/video/providers/webdav/webdav_bridge.hpp"
#include "../../providers/http/test_fixture.hpp"
#include "source_core/test_helpers.hpp"
using namespace kivo::video::source_core; using namespace kivo::video::source_core::test;
namespace wd=kivo::video::providers::webdav; namespace ph=kivo::video::providers::http;
// PROPFIND body_cap_hit → probe_resource_type returns status-based result (207→file_like)
// body_cap_hit is checked at the HTTP Range open level, not in PROPFIND probe
static ph::HttpTransportFn fx_xml_cap(){return[](const ph::HttpTransportRequest& r)->ph::HttpTransportResponse{ph::HttpTransportResponse o;if(r.method=="PROPFIND"){o.status=207;o.body_cap_hit=true;}return o;};}
int main(){
    auto info=wd::probe_resource_type("http://x.com/f",fx_xml_cap());
    // 207 → file_like; body_cap_hit checked at HttpRangeProvider level
    CHECK_EQ(info.probe,wd::WebDavProbeResult::file_like);
    // Opening via bridge: body_cap_hit causes failure at HTTP level
    ph::HttpRangeSessionStore s;
    auto r=wd::WebDavRangeBridge::open(SourceOpenRequest{SourceUri::from_raw("http://x.com/f")},fx_xml_cap(),s);
    CHECK_TRUE(r.is_failure());
    return 0;
}
