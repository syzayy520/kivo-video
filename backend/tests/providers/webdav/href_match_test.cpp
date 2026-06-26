#include "kivo/video/providers/webdav/webdav_bridge.hpp"
#include "../../providers/http/test_fixture.hpp"
#include "source_core/test_helpers.hpp"
using namespace kivo::video::source_core; using namespace kivo::video::source_core::test;
namespace wd=kivo::video::providers::webdav; namespace ph=kivo::video::providers::http;
static ph::HttpTransportFn fx_href(){return[](const ph::HttpTransportRequest& r)->ph::HttpTransportResponse{ph::HttpTransportResponse o;if(r.method=="PROPFIND"){o.status=207;o.headers["Content-Type"]="application/xml; charset=utf-8";}else if(r.range_start.has_value()){o.status=206;o.headers["Content-Range"]="bytes 0-0/6";o.body.push_back(static_cast<std::uint8_t>('a'));}return o;};}
int main(){ph::HttpRangeSessionStore s;auto o=wd::WebDavRangeBridge::open(SourceOpenRequest{SourceUri::from_raw("http://x.com/f")},fx_href(),s);CHECK_TRUE(o.is_success());return 0;}
