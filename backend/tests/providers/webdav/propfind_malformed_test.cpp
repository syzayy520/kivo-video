#include "kivo/video/providers/webdav/webdav_bridge.hpp"
#include "../../providers/http/test_fixture.hpp"
#include "source_core/test_helpers.hpp"
using namespace kivo::video::source_core; using namespace kivo::video::source_core::test;
namespace wd=kivo::video::providers::webdav; namespace ph=kivo::video::providers::http;
static ph::HttpTransportFn fx_malformed(){return[](const ph::HttpTransportRequest& r)->ph::HttpTransportResponse{ph::HttpTransportResponse o;if(r.method=="PROPFIND"){o.status=400;o.headers["Content-Type"]="text/html; charset=utf-8";o.body.push_back('x');}return o;};}
int main(){auto info=wd::probe_resource_type("http://x.com/f",fx_malformed());CHECK_EQ(info.probe,wd::WebDavProbeResult::resource_type_unknown);return 0;}
