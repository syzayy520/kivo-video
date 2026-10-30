#include "kivo/playback/control_plane/primitives/value_types.hpp"
#include "kivo/playback/control_plane/primitives/ratio.hpp"
#include "kivo/playback/control_plane/rate/playback_rate_ratio.hpp"
#include "kivo/playback/control_plane/primitives/time_types.hpp"
#include "kivo/playback/control_plane/primitives/source_range.hpp"
#include "kivo/playback/control_plane/primitives/stream_type.hpp"
#include "kivo/playback/control_plane/revision/revision_master_model.hpp"
#include "kivo/playback/control_plane/revision/callback_token.hpp"
#include "kivo/playback/control_plane/revision/revision_stamp.hpp"
#include "kivo/playback/control_plane/revision/snapshot_stamp.hpp"
#include "kivo/playback/control_plane/state/session_state.hpp"

#include <cassert>
#include <iostream>

using namespace kivo::playback::control_plane;

static void test_milliseconds_strong_type()
{
    primitives::Milliseconds a{100};
    primitives::Milliseconds b{200};
    auto c = a.checked_add(b);
    assert(c.value == 300);
    auto d = a.checked_sub(b);
    assert(d.value == 0);
    auto e = a.saturating_add(b);
    assert(e.has_value());
    assert(e->value == 300);
    std::cout << "PASS: milliseconds_strong_type\n";
}

static void test_bytes_strong_type()
{
    primitives::Bytes a{1024};
    primitives::Bytes b{2048};
    auto c = a.checked_add(b);
    assert(c.value == 3072);
    std::cout << "PASS: bytes_strong_type\n";
}

static void test_bps_to_bps_conversion()
{
    primitives::BitsPerSecond bps{8000};
    auto result = primitives::BytesPerSecond::from_bps(bps);
    assert(result.has_value());
    assert(result->value == 1000);
    primitives::BitsPerSecond odd{8001};
    auto odd_result = primitives::BytesPerSecond::from_bps(odd);
    assert(!odd_result.has_value());
    std::cout << "PASS: bps_to_bps_conversion\n";
}

static void test_count_strong_type()
{
    primitives::Count a{5};
    primitives::Count b{3};
    auto c = a.checked_add(b);
    assert(c.value == 8);
    std::cout << "PASS: count_strong_type\n";
}

static void test_ratio_canonicalization()
{
    auto r1 = primitives::Ratio::make(primitives::U32{2}, primitives::U32{2});
    assert(r1.has_value());
    assert(r1->numerator == 1);
    assert(r1->denominator == 1);
    auto r2 = primitives::Ratio::make(primitives::U32{5}, primitives::U32{4});
    assert(r2->numerator == 5);
    assert(r2->denominator == 4);
    auto r_zero = primitives::Ratio::make(primitives::U32{1}, primitives::U32{0});
    assert(!r_zero.has_value());
    std::cout << "PASS: ratio_canonicalization\n";
}

static void test_playback_rate_ratio()
{
    auto normal = rate::PlaybackRateRatio::make(primitives::U32{1}, primitives::U32{1});
    assert(normal.has_value());
    assert(normal->is_normal_rate());
    auto canonical = rate::PlaybackRateRatio::make(primitives::U32{2}, primitives::U32{2});
    assert(canonical.has_value());
    assert(canonical->is_normal_rate());
    auto fast = rate::PlaybackRateRatio::make(primitives::U32{5}, primitives::U32{4});
    assert(fast.has_value());
    assert(!fast->is_normal_rate());
    std::cout << "PASS: playback_rate_ratio\n";
}

static void test_deadline_infinite()
{
    auto d = primitives::Deadline::make_infinite();
    assert(d.infinite);
    primitives::MonotonicTimestamp t{5000};
    primitives::Deadline finite{t};
    assert(!finite.infinite);
    assert(finite.at.ticks_ns == 5000);
    std::cout << "PASS: deadline_infinite\n";
}

static void test_media_time_vod()
{
    primitives::MediaTimeMs valid{1000};
    assert(valid.is_valid_vod_absolute());
    primitives::MediaTimeMs invalid_val{-1};
    assert(!invalid_val.is_valid_vod_absolute());
    std::cout << "PASS: media_time_vod\n";
}

static void test_source_range()
{
    primitives::SourceRange range{primitives::SourceOffset{primitives::Bytes{0}}, primitives::SourceOffset{primitives::Bytes{1024}}};
    assert(range.is_valid());
    assert(!range.is_empty());
    assert(range.size().value == 1024);
    primitives::SourceRange empty_range{primitives::SourceOffset{primitives::Bytes{0}}, primitives::SourceOffset{primitives::Bytes{0}}};
    assert(empty_range.is_empty());
    std::cout << "PASS: source_range\n";
}

static void test_id_types_distinct()
{
    revision::SessionId sid{42};
    revision::CommandId cid{100};
    revision::ReaderInstanceId rid{7};
    assert(sid.value == 42);
    assert(cid.value == 100);
    assert(rid.value == 7);
    std::cout << "PASS: id_types_distinct\n";
}

static void test_epoch_types()
{
    revision::SessionGeneration gen{1};
    revision::ReadEpoch re{2};
    revision::SeekEpoch se{3};
    revision::RecoveryEpoch rce{4};
    revision::StreamRevision sr{5};
    revision::TimelineRevision tr{6};
    revision::P4RuntimePolicyRevision rpr{7};
    assert(gen.value == 1 && re.value == 2 && se.value == 3);
    assert(rce.value == 4 && sr.value == 5 && tr.value == 6 && rpr.value == 7);
    std::cout << "PASS: epoch_types\n";
}

static void test_revision_stamp_all_fields()
{
    revision::RevisionStamp stamp;
    stamp.session_id = revision::SessionId{1};
    stamp.generation = revision::SessionGeneration{1};
    stamp.read_epoch = revision::ReadEpoch{1};
    stamp.seek_epoch = revision::SeekEpoch{1};
    stamp.recovery_epoch = revision::RecoveryEpoch{1};
    stamp.stream_revision = revision::StreamRevision{1};
    stamp.timeline_revision = revision::TimelineRevision{1};
    stamp.runtime_policy_revision = revision::P4RuntimePolicyRevision{1};
    stamp.callback_token = revision::CallbackToken{1};
    assert(stamp.session_id.value == 1);
    assert(stamp.callback_token.nonce == 1);
    std::cout << "PASS: revision_stamp_all_fields\n";
}

static void test_snapshot_stamp_fields()
{
    revision::SnapshotStamp ss;
    ss.session_id = revision::SessionId{1};
    ss.captured_at = primitives::MonotonicTimestamp{9999};
    assert(ss.session_id.value == 1);
    assert(ss.captured_at.ticks_ns == 9999);
    std::cout << "PASS: snapshot_stamp_fields\n";
}

static void test_callback_token_nonce_only()
{
    revision::CallbackToken t1{1};
    revision::CallbackToken t2{2};
    assert(t1.nonce == 1);
    assert(t2.nonce == 2);
    std::cout << "PASS: callback_token_nonce_only\n";
}

static void test_session_state_valid_transitions()
{
    namespace s = state;
    assert(s::is_valid_transition(s::SessionState::Idle, s::SessionState::Initializing));
    assert(s::is_valid_transition(s::SessionState::Idle, s::SessionState::Terminated));
    assert(s::is_valid_transition(s::SessionState::Initializing, s::SessionState::Ready));
    assert(s::is_valid_transition(s::SessionState::Ready, s::SessionState::Playing));
    assert(s::is_valid_transition(s::SessionState::Playing, s::SessionState::Paused));
    assert(s::is_valid_transition(s::SessionState::Playing, s::SessionState::Seeking));
    assert(s::is_valid_transition(s::SessionState::Paused, s::SessionState::Playing));
    assert(s::is_valid_transition(s::SessionState::Seeking, s::SessionState::Playing));
    assert(s::is_valid_transition(s::SessionState::Buffering, s::SessionState::Playing));
    assert(s::is_valid_transition(s::SessionState::ErrorRecovery, s::SessionState::Ready));
    assert(s::is_valid_transition(s::SessionState::Stopping, s::SessionState::Terminated));
    std::cout << "PASS: session_state_valid_transitions\n";
}

static void test_session_state_invalid_transitions()
{
    namespace s = state;
    assert(!s::is_valid_transition(s::SessionState::Idle, s::SessionState::Playing));
    assert(!s::is_valid_transition(s::SessionState::Idle, s::SessionState::Ready));
    assert(!s::is_valid_transition(s::SessionState::Ready, s::SessionState::Buffering));
    assert(!s::is_valid_transition(s::SessionState::Paused, s::SessionState::Buffering));
    assert(!s::is_valid_transition(s::SessionState::Terminated, s::SessionState::Idle));
    assert(!s::is_valid_transition(s::SessionState::Terminated, s::SessionState::Playing));
    assert(!s::is_valid_transition(s::SessionState::Stopping, s::SessionState::Playing));
    std::cout << "PASS: session_state_invalid_transitions\n";
}

static void test_session_state_all_100_combinations()
{
    namespace s = state;
    s::SessionState states[] = {
        s::SessionState::Idle, s::SessionState::Initializing, s::SessionState::Ready,
        s::SessionState::Playing, s::SessionState::Paused, s::SessionState::Seeking,
        s::SessionState::Buffering, s::SessionState::ErrorRecovery,
        s::SessionState::Stopping, s::SessionState::Terminated
    };
    int valid_count = 0;
    int invalid_count = 0;
    for (int i = 0; i < 10; ++i)
    {
        for (int j = 0; j < 10; ++j)
        {
            if (i == j) continue;
            if (s::is_valid_transition(states[i], states[j]))
                ++valid_count;
            else
                ++invalid_count;
        }
    }
    assert(valid_count > 0);
    assert(invalid_count > 0);
    assert(valid_count + invalid_count == 90);
    std::cout << "PASS: session_state_all_100_combinations (valid=" << valid_count << " invalid=" << invalid_count << ")\n";
}

int main()
{
    test_milliseconds_strong_type();
    test_bytes_strong_type();
    test_bps_to_bps_conversion();
    test_count_strong_type();
    test_ratio_canonicalization();
    test_playback_rate_ratio();
    test_deadline_infinite();
    test_media_time_vod();
    test_source_range();
    test_id_types_distinct();
    test_epoch_types();
    test_revision_stamp_all_fields();
    test_snapshot_stamp_fields();
    test_callback_token_nonce_only();
    test_session_state_valid_transitions();
    test_session_state_invalid_transitions();
    test_session_state_all_100_combinations();
    std::cout << "ALL PASS: p4_primitives_revision_tests (17 tests)\n";
    return 0;
}
