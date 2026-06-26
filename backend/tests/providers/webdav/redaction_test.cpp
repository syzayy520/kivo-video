#include "kivo/video/providers/webdav/webdav_bridge.hpp"
#include "../../providers/http/test_fixture.hpp"
#include "source_core/test_helpers.hpp"
#include <string>
using namespace kivo::video::source_core; using namespace kivo::video::source_core::test;
namespace wd=kivo::video::providers::webdav; namespace ph=kivo::video::providers::http;
static ph::HttpTransportFn fx(){return[](const ph::HttpTransportRequest& r)->ph::HttpTransportResponse{ph::HttpTransportResponse o;if(r.method=="PROPFIND"){o.status=405;}else if(r.range_start.has_value()){o.status=206;o.headers["Content-Range"]="bytes 0-0/10";}return o;};}
int main(){ph::HttpRangeSessionStore s;auto o=wd::WebDavRangeBridge::open(SourceOpenRequest{SourceUri::from_raw("http://example.com/v.mp4")},fx(),s);CHECK_TRUE(o.is_success());auto ev=wd::WebDavRangeBridge::get_evidence(o.session_if_success()->session_id,s);for(auto&i:ev.items)CHECK_TRUE(i.safe_description.find("http://example.com")==std::string::npos);return 0;}
