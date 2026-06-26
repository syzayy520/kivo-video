#include "kivo/video/providers/webdav/webdav_bridge.hpp"
#include "../../providers/http/test_fixture.hpp"
#include "source_core/test_helpers.hpp"
using namespace kivo::video::source_core; using namespace kivo::video::source_core::test;
namespace wd=kivo::video::providers::webdav; namespace ph=kivo::video::providers::http;
// PROPFIND returns 401 → open fails with auth error
static ph::HttpTransportFn fx_auth_fail(){return[](const ph::HttpTransportRequest& r)->ph::HttpTransportResponse{ph::HttpTransportResponse o;if(r.method=="PROPFIND"){o.status=401;o.headers["WWW-Authenticate"]="Basic realm=\"test\"";}else{o.status=401;}return o;};}
int main(){
    ph::HttpRangeSessionStore s;
    auto r=wd::WebDavRangeBridge::open(SourceOpenRequest{SourceUri::from_raw("http://x.com/f")},fx_auth_fail(),s);
    CHECK_TRUE(r.is_failure());
    return 0;
}
