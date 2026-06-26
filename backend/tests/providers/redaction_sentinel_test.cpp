#include "kivo/video/providers/http/http_range_provider.hpp"
#include "kivo/video/providers/webdav/webdav_bridge.hpp"
#include "../../tests/providers/http/test_fixture.hpp"
#include "source_core/test_helpers.hpp"
#include <string>
using namespace kivo::video::source_core; using namespace kivo::video::source_core::test;
namespace ph=kivo::video::providers::http; namespace wd=kivo::video::providers::webdav;

// RR-027/031: Redaction Sentinel Capture Test
// Prove that URL, auth tokens, paths NEVER leak into evidence output
int main() {
    // HTTP: open with auth-bearing URL → verify evidence has no raw URL or auth
    auto fx = make_fixture(FixtureMode::range_ok, 6, "abcdef");
    ph::HttpRangeSessionStore s;

    // Test 1: HTTP evidence must not contain raw URL
    auto o1 = ph::HttpRangeProvider::open(
        SourceOpenRequest{SourceUri::from_raw("http://example.com/video.mp4")}, fx, s);
    CHECK_TRUE(o1.is_success());
    auto ev1 = ph::HttpRangeProvider::get_evidence(o1.session_if_success()->session_id, s);
    for (auto& item : ev1.items) {
        CHECK_TRUE(item.safe_description.find("http://example.com") == std::string::npos);
    }

    // Test 2: HTTP evidence must not contain auth-like tokens
    auto o2 = ph::HttpRangeProvider::open(
        SourceOpenRequest{SourceUri::from_raw("http://x.com/f?token=sekret123&key=abc")}, fx, s);
    CHECK_TRUE(o2.is_success());
    auto ev2 = ph::HttpRangeProvider::get_evidence(o2.session_if_success()->session_id, s);
    for (auto& item : ev2.items) {
        CHECK_TRUE(item.safe_description.find("token=sekret123") == std::string::npos);
        CHECK_TRUE(item.safe_description.find("key=abc") == std::string::npos);
    }

    // Test 3: WebDAV evidence must not contain raw URL or path
    auto wdfx = [](const ph::HttpTransportRequest& r) -> ph::HttpTransportResponse {
        ph::HttpTransportResponse o;
        if (r.method == "PROPFIND") { o.status = 405; }
        else if (r.range_start.has_value()) {
            o.status = 206;
            o.headers["Content-Range"] = "bytes 0-0/10";
        }
        return o;
    };
    auto o3 = wd::WebDavRangeBridge::open(
        SourceOpenRequest{SourceUri::from_raw("http://server.com/webdav/path/file.mkv")},
        wdfx, s);
    CHECK_TRUE(o3.is_success());
    auto ev3 = wd::WebDavRangeBridge::get_evidence(o3.session_if_success()->session_id, s);
    for (auto& item : ev3.items) {
        CHECK_TRUE(item.safe_description.find("http://server.com") == std::string::npos);
        CHECK_TRUE(item.safe_description.find("/webdav/path") == std::string::npos);
        CHECK_TRUE(item.safe_description.find("file.mkv") == std::string::npos);
    }

    std::cout << "REDACTION_SENTINEL_GATE: PASS" << std::endl;
    return 0;
}
