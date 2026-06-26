#include "kivo/video/providers/webdav/webdav_bridge.hpp"
#include "../../providers/http/test_fixture.hpp"
#include "source_core/test_helpers.hpp"
using namespace kivo::video::source_core; using namespace kivo::video::source_core::test;
namespace wd=kivo::video::providers::webdav; namespace ph=kivo::video::providers::http;
static ph::HttpTransportFn fx(){return[](const ph::HttpTransportRequest& r)->ph::HttpTransportResponse{ph::HttpTransportResponse o;if(r.method=="PROPFIND"){o.status=405;}else if(r.range_start.has_value()&&r.range_end.has_value()){auto s=*r.range_start;auto e=*r.range_end;o.status=206;auto len=std::min(e-s+1,6ULL-s);o.headers["Content-Range"]="bytes "+std::to_string(s)+"-"+std::to_string(s+len-1)+"/6";std::string b="abcdef";if(s<6){auto en=std::min(6ULL,s+len);o.body.assign(b.begin()+s,b.begin()+en);}}return o;};}
int main(){ph::HttpRangeSessionStore s;auto o=wd::WebDavRangeBridge::open(SourceOpenRequest{SourceUri::from_raw("http://x.com/f")},fx(),s);CHECK_TRUE(o.is_success());auto sid=o.session_if_success()->session_id;auto rr=wd::WebDavRangeBridge::read(SourceReadRequest{sid,std::nullopt,3},fx(),s);CHECK_TRUE(rr.is_success());CHECK_EQ(rr.bytes[0],(std::uint8_t)'a');return 0;}
