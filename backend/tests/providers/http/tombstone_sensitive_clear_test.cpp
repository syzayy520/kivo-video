#include "kivo/video/providers/http/http_range_provider.hpp"
#include "source_core/test_helpers.hpp"
#include "test_fixture.hpp"
using namespace kivo::video::source_core; using namespace kivo::video::source_core::test;

// RR-023/063: Tombstone sensitive clear test
// Prove raw_url is zeroed after close via tombstone_sensitive_clear()
int main() {
    auto fx = make_fixture(FixtureMode::range_ok, 6, "abcdef");
    ph::HttpRangeSessionStore s;
    auto o = ph::HttpRangeProvider::open(
        SourceOpenRequest{SourceUri::from_raw("http://example.com/secret.mp4")}, fx, s);
    CHECK_TRUE(o.is_success());
    auto id = o.session_if_success()->session_id;

    // Verify raw_url populated before close
    auto pre = s.snapshot(id);
    CHECK_TRUE(pre.has_value());
    CHECK_TRUE(!pre->raw_url.empty());

    // Close → tombstone_sensitive_clear
    ph::HttpRangeProvider::close(id, s);

    // After close, tombstone should have raw_url cleared
    auto post = s.snapshot(id);
    CHECK_TRUE(post.has_value());
    CHECK_TRUE(post->raw_url.empty());

    // But redacted_url (evidence-safe) still accessible
    CHECK_TRUE(!post->redacted_url.empty());
    CHECK_TRUE(post->redacted_url.find("/secret.mp4") == std::string::npos);

    // ProviderInternalRemoteUri is move-only with secure_clear
    {
        ph::ProviderInternalRemoteUri test_uri("sensitive_data_12345");
        // Move-only proof
        ph::ProviderInternalRemoteUri moved_uri(std::move(test_uri));
        CHECK_TRUE(test_uri.raw.empty());
        CHECK_TRUE(!moved_uri.raw.empty());
        moved_uri.secure_clear();
        CHECK_TRUE(moved_uri.raw.empty());
    }

    std::cout << "TOMBSTONE_SENSITIVE_CLEAR_GATE: PASS" << std::endl;
    return 0;
}
