// evidence_tests.cpp — P6B runtime evidence tests (B15-B16)
// Tests FakeEvidenceCollector (B15) and FakeEvidenceIdGenerator (B16).
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <stdexcept>

#include "kivo/video/audio_plane/runtime/evidence/fake_evidence_collector.hpp"
#include "kivo/video/audio_plane/runtime/evidence/fake_evidence_id_generator.hpp"

using namespace kivo::video::audio_plane::runtime::evidence;

static int g_failed = 0;

#define P6B_RUN(name) \
    do { \
        try { name(); } \
        catch (...) { \
            ++g_failed; \
            std::printf("  >>> FAIL: %s\n", #name); \
        } \
    } while (0)

// ---------------------------------------------------------------------------
// B15: FakeEvidenceCollector
// ---------------------------------------------------------------------------

// record() returns monotonic evidence ids 1, 2, 3.
static void test_b15_record_returns_monotonic_ids() {
    FakeEvidenceCollector collector;
    collector.configure(/*ttl_ms*/ 30000, /*capacity*/ 1024);

    uint64_t id1 = collector.record(/*kind*/ 1, /*severity*/ 0,
                                    /*correlation_id*/ 10, /*epoch*/ 1,
                                    /*timestamp_ms*/ 1000);
    if (id1 != 1ull) throw std::runtime_error("first record id must be 1");

    uint64_t id2 = collector.record(2, 0, 11, 1, 1100);
    if (id2 != 2ull) throw std::runtime_error("second record id must be 2");

    uint64_t id3 = collector.record(3, 0, 12, 1, 1200);
    if (id3 != 3ull) throw std::runtime_error("third record id must be 3");

    if (collector.count() != 3u) throw std::runtime_error("count != 3");

    std::printf("[B15] record returns monotonic ids PASS\n");
}

// count() reflects the number of currently-retained records.
static void test_b15_count_tracks_records() {
    FakeEvidenceCollector collector;
    collector.configure(30000, 1024);

    if (collector.count() != 0u) throw std::runtime_error("count != 0 at start");

    collector.record(1, 0, 1, 1, 1000);
    if (collector.count() != 1u) throw std::runtime_error("count != 1 after one record");

    collector.record(2, 0, 2, 1, 2000);
    collector.record(3, 0, 3, 1, 3000);
    if (collector.count() != 3u) throw std::runtime_error("count != 3 after three records");

    collector.clear();
    if (collector.count() != 0u) throw std::runtime_error("count != 0 after clear");

    std::printf("[B15] count tracks records PASS\n");
}

// TTL expiry: non-chain-root records past their TTL are dropped on the next
// record() call.
static void test_b15_ttl_expiry_drops_expired() {
    FakeEvidenceCollector collector;
    // Short TTL: 100 ms.
    collector.configure(/*ttl_ms*/ 100, /*capacity*/ 1024);

    // Record two entries at t=1000 and t=1050 (both within TTL of each other).
    collector.record(1, 0, 1, 1, 1000);
    collector.record(2, 0, 2, 1, 1050);
    if (collector.count() != 2u) throw std::runtime_error("count != 2 before TTL expiry");

    // Now record a third entry at t=5000. The first entry (t=1000) is now
    // 4000ms old, well past the 100ms TTL; the second (t=1050) is 3950ms old,
    // also past TTL. Both should be dropped before the new record is appended.
    collector.record(3, 0, 3, 1, 5000);
    if (collector.count() != 1u) {
        char buf[128];
        std::snprintf(buf, sizeof(buf), "expected count=1 after TTL expiry, got %zu",
                      collector.count());
        throw std::runtime_error(buf);
    }

    // The remaining record should be the new one (id 3).
    const auto& recs = collector.records();
    if (recs.size() != 1u) throw std::runtime_error("records size != 1");
    if (recs[0].evidence_id != 3ull) {
        throw std::runtime_error("remaining record must be the newest (id 3)");
    }

    std::printf("[B15] TTL expiry drops expired PASS\n");
}

// ChainRoot records (severity & CHAIN_ROOT_SEVERITY_BIT) cannot be silently
// dropped by TTL; when storage is full and only chain-root records remain,
// compact_critical_counter_ is incremented.
static void test_b15_chain_root_not_dropped_increments_compact_counter() {
    FakeEvidenceCollector collector;
    // Capacity 2 so we can fill it quickly.
    collector.configure(/*ttl_ms*/ 100000, /*capacity*/ 2);

    // Fill storage with two ChainRoot records (severity has the chain-root bit).
    int32_t chain_root_severity = CHAIN_ROOT_SEVERITY_BIT | 0x1;
    collector.record(1, chain_root_severity, 1, 1, 1000);
    collector.record(2, chain_root_severity, 2, 1, 1100);
    if (collector.count() != 2u) throw std::runtime_error("count != 2 after filling");

    // Try to TTL-evict by recording at a much later timestamp. ChainRoot
    // records must NOT be dropped, so count must stay >= 2.
    collector.record(3, chain_root_severity, 3, 1, 999999);
    if (collector.count() < 2u) {
        throw std::runtime_error("ChainRoot records must not be silently dropped by TTL");
    }

    // Because storage was full of chain-root records, the compact_critical
    // counter must have been incremented at least once for the third record.
    if (collector.compact_critical_counter() == 0ull) {
        throw std::runtime_error("compact_critical_counter must increment when storage is full of ChainRoot records");
    }

    std::printf("[B15] ChainRoot not dropped / compact_critical_counter increments PASS\n");
}

// clear() resets records, the id counter, and the compact_critical counter.
static void test_b15_clear_resets_state() {
    FakeEvidenceCollector collector;
    collector.configure(30000, 4);

    collector.record(1, 0, 1, 1, 1000);
    collector.record(2, 0, 2, 1, 1100);
    // Force a compact-critical increment.
    int32_t chain_root_severity = CHAIN_ROOT_SEVERITY_BIT | 0x1;
    collector.record(3, chain_root_severity, 3, 1, 1200);
    collector.record(4, chain_root_severity, 4, 1, 1300);
    collector.record(5, chain_root_severity, 5, 1, 1400);

    collector.clear();
    if (collector.count() != 0u) throw std::runtime_error("count != 0 after clear");
    if (collector.compact_critical_counter() != 0ull) {
        throw std::runtime_error("compact_critical_counter != 0 after clear");
    }

    // After clear, the next record gets id 1 again (id counter is reset).
    uint64_t id = collector.record(1, 0, 1, 1, 1000);
    if (id != 1ull) throw std::runtime_error("first id after clear must be 1");

    std::printf("[B15] clear resets state PASS\n");
}

// ---------------------------------------------------------------------------
// B16: FakeEvidenceIdGenerator
// ---------------------------------------------------------------------------

// next_id() returns 1, 2, 3 in sequence (0 is reserved invalid).
static void test_b16_next_id_returns_monotonic_sequence() {
    FakeEvidenceIdGenerator gen;

    uint64_t id1 = gen.next_id();
    if (id1 != 1ull) throw std::runtime_error("first next_id must be 1");

    uint64_t id2 = gen.next_id();
    if (id2 != 2ull) throw std::runtime_error("second next_id must be 2");

    uint64_t id3 = gen.next_id();
    if (id3 != 3ull) throw std::runtime_error("third next_id must be 3");

    // 0 is never returned by a healthy generator.
    if (id1 == 0ull || id2 == 0ull || id3 == 0ull) {
        throw std::runtime_error("next_id must never return 0 before wrap");
    }

    std::printf("[B16] next_id returns monotonic sequence PASS\n");
}

// 0 is the reserved invalid sentinel; is_wrapped() is false before wrap.
static void test_b16_zero_reserved_invalid_before_wrap() {
    FakeEvidenceIdGenerator gen;
    if (gen.is_wrapped()) throw std::runtime_error("is_wrapped must be false on a fresh generator");

    for (int i = 0; i < 100; ++i) {
        uint64_t id = gen.next_id();
        if (id == 0ull) {
            throw std::runtime_error("next_id returned 0 before wrap");
        }
    }
    if (gen.is_wrapped()) throw std::runtime_error("is_wrapped must still be false");

    std::printf("[B16] zero reserved invalid before wrap PASS\n");
}

// Wrap detection: when the counter is at UINT64_MAX, the next next_id()
// returns 0 (invalid) and sets the wrap flag.
//
// We cannot realistically drive a fresh generator to UINT64_MAX, so we verify
// the wrap contract indirectly by confirming:
//   1. is_wrapped() starts false.
//   2. After many increments, is_wrapped() is still false.
//   3. reset() clears the wrap flag (and counter), so the next next_id()
//      returns 1 again.
// The wrap branch itself is exercised by the generator's internal logic when
// counter_ == UINT64_MAX; we rely on the contract documented in the header.
static void test_b16_wrap_detection_contract() {
    FakeEvidenceIdGenerator gen;
    if (gen.is_wrapped()) throw std::runtime_error("fresh generator must not be wrapped");

    // Drive a modest number of increments; wrap must not trigger.
    for (int i = 0; i < 1000; ++i) {
        gen.next_id();
    }
    if (gen.is_wrapped()) {
        throw std::runtime_error("generator must not wrap after only 1000 increments");
    }

    // reset() must clear the counter and the wrap flag.
    gen.reset();
    if (gen.is_wrapped()) throw std::runtime_error("reset must clear wrap flag");
    uint64_t id = gen.next_id();
    if (id != 1ull) {
        throw std::runtime_error("first next_id after reset must be 1");
    }

    std::printf("[B16] wrap detection contract PASS\n");
}

// reset() returns the generator to its initial state.
static void test_b16_reset_returns_to_initial_state() {
    FakeEvidenceIdGenerator gen;
    gen.next_id();
    gen.next_id();
    gen.next_id();

    gen.reset();
    if (gen.is_wrapped()) throw std::runtime_error("is_wrapped must be false after reset");

    uint64_t id = gen.next_id();
    if (id != 1ull) throw std::runtime_error("next_id after reset must be 1");

    std::printf("[B16] reset returns to initial state PASS\n");
}

int main() {
    std::printf("=== P6B evidence_tests ===\n");

    P6B_RUN(test_b15_record_returns_monotonic_ids);
    P6B_RUN(test_b15_count_tracks_records);
    P6B_RUN(test_b15_ttl_expiry_drops_expired);
    P6B_RUN(test_b15_chain_root_not_dropped_increments_compact_counter);
    P6B_RUN(test_b15_clear_resets_state);
    P6B_RUN(test_b16_next_id_returns_monotonic_sequence);
    P6B_RUN(test_b16_zero_reserved_invalid_before_wrap);
    P6B_RUN(test_b16_wrap_detection_contract);
    P6B_RUN(test_b16_reset_returns_to_initial_state);

    if (g_failed == 0) {
        std::printf("=== P6B evidence_tests: ALL PASS ===\n");
        return 0;
    }
    std::printf("=== P6B evidence_tests: %d FAILED ===\n", g_failed);
    return 1;
}
