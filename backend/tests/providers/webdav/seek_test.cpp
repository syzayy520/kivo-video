#include "kivo/video/providers/webdav/webdav_bridge.hpp"
#include "../../providers/http/test_fixture.hpp"
#include "source_core/test_helpers.hpp"
using namespace kivo::video::source_core; using namespace kivo::video::source_core::test;
namespace wd=kivo::video::providers::webdav; namespace ph=kivo::video::providers::http;
static ph::HttpTransportFn fx(){return[](const ph::HttpTransportRequest& r)->ph::HttpTransportResponse{ph::HttpTransportResponse o;if(r.method=="PROPFIND"){o.status=405;}else if(r.range_start.has_value()){o.status=206;o.headers["Content-Range"]="bytes 0-0/6";}return o;};}
int main(){ph::HttpRangeSessionStore s;auto o=wd::WebDavRangeBridge::open(SourceOpenRequest{SourceUri::from_raw("http://x.com/f")},fx(),s);CHECK_TRUE(o.is_success());auto id=o.session_if_success()->session_id;SourceSeekRequest sr{id,2,SeekOrigin::begin};auto rr=wd::WebDavRangeBridge::seek(sr,s);CHECK_TRUE(rr.is_success());CHECK_EQ(rr.new_position.value(),2ULL);return 0;}
