#include "kivo/video/providers/webdav/webdav_bridge.hpp"
#include "../../providers/http/test_fixture.hpp"
#include "source_core/test_helpers.hpp"
using namespace kivo::video::source_core; using namespace kivo::video::source_core::test;
namespace wd=kivo::video::providers::webdav; namespace ph=kivo::video::providers::http;
static ph::HttpTransportFn fx(){return[](const ph::HttpTransportRequest& r)->ph::HttpTransportResponse{ph::HttpTransportResponse o;if(r.method=="PROPFIND"){o.status=405;}else if(r.range_start.has_value()){auto s=r.range_start.value();auto e=r.range_end.value();o.status=206;auto len=std::min(e-s+1,6ULL-s);o.headers["Content-Range"]="bytes "+std::to_string(s)+"-"+std::to_string(s+len-1)+"/6";}return o;};}
int main(){ph::HttpRangeSessionStore s;auto o=wd::WebDavRangeBridge::open(SourceOpenRequest{SourceUri::from_raw("http://x.com/f")},fx(),s);CHECK_TRUE(o.is_success());auto id=o.session_if_success()->session_id;auto rr=wd::WebDavRangeBridge::read(SourceReadRequest{id,2ULL,1},fx(),s);CHECK_TRUE(rr.is_success());auto off=s.snapshot(id)->current_offset;CHECK_EQ(off,0ULL);return 0;}
