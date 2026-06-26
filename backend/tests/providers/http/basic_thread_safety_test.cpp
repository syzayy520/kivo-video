#include "kivo/video/providers/http/http_range_provider.hpp"
#include "source_core/test_helpers.hpp"
#include <thread>
#include <atomic>
using namespace kivo::video::source_core;
namespace ph = kivo::video::providers::http;
static ph::HttpTransportFn fx(uint64_t t){return [=](const ph::HttpTransportRequest& r)->ph::HttpTransportResponse{ph::HttpTransportResponse o;if(r.range_start.has_value()&&r.range_start.value()<t){o.status=206;o.headers["Content-Range"]="bytes "+std::to_string(r.range_start.value())+"-0/"+std::to_string(t);}return o;};}
int main(){ph::HttpRangeSessionStore s;auto o=ph::HttpRangeProvider::open(SourceOpenRequest{SourceUri::from_raw("http://x.com/f")},fx(100),s);CHECK_TRUE(o.is_success());auto id=o.session_if_success()->session_id;std::atomic<bool> run{true};std::atomic<int> err{0};
auto t1=[&]{while(run)ph::HttpRangeProvider::get_evidence(id,s);};
auto t2=[&]{while(run)ph::HttpRangeProvider::read(SourceReadRequest{id,10ULL,4},fx(100),s);};
std::thread a(t1),b(t2);std::this_thread::sleep_for(std::chrono::milliseconds(50));run=false;a.join();b.join();CHECK_EQ(err.load(),0);
ph::HttpRangeProvider::close(id,s);return 0;}
