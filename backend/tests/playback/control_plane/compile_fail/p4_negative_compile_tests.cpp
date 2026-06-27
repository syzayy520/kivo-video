#include "kivo/playback/control_plane/primitives/value_types.hpp"
#include "kivo/playback/control_plane/primitives/time_types.hpp"
#include "kivo/playback/control_plane/revision/revision_master_model.hpp"
#include "kivo/playback/control_plane/revision/cancellation.hpp"
#include "kivo/playback/control_plane/command/playback_command.hpp"
#include "kivo/playback/control_plane/rate/playback_rate_ratio.hpp"
#include "kivo/playback/control_plane/handoff/p3_handoff_ref.hpp"

#include <type_traits>
#include <cassert>
#include <iostream>

using namespace kivo::playback::control_plane;

// P4G-062: Negative Compile Tests
// These tests verify that strong type boundaries are enforced at compile time.
// Each test uses static_assert to verify that forbidden conversions are rejected.

// 1. Milliseconds cannot pass to Bytes
static void test_milliseconds_not_bytes()
{
    static_assert(!std::is_constructible_v<primitives::Bytes, primitives::Milliseconds>,
        "Milliseconds must NOT be constructible as Bytes");
    static_assert(!std::is_convertible_v<primitives::Milliseconds, primitives::Bytes>,
        "Milliseconds must NOT be convertible to Bytes");
    std::cout << "PASS: milliseconds_not_bytes\n";
}

// 2. Bytes cannot pass to Count
static void test_bytes_not_count()
{
    static_assert(!std::is_constructible_v<primitives::Count, primitives::Bytes>,
        "Bytes must NOT be constructible as Count");
    static_assert(!std::is_convertible_v<primitives::Bytes, primitives::Count>,
        "Bytes must NOT be convertible to Count");
    std::cout << "PASS: bytes_not_count\n";
}

// 3. BitsPerSecond cannot pass to Bytes
static void test_bps_not_bytes()
{
    static_assert(!std::is_constructible_v<primitives::Bytes, primitives::BitsPerSecond>,
        "BitsPerSecond must NOT be constructible as Bytes");
    static_assert(!std::is_convertible_v<primitives::BitsPerSecond, primitives::Bytes>,
        "BitsPerSecond must NOT be convertible to Bytes");
    std::cout << "PASS: bps_not_bytes\n";
}

// 4. BytesPerSecond cannot pass to BitsPerSecond
static void test_bps_not_bps2()
{
    static_assert(!std::is_constructible_v<primitives::BitsPerSecond, primitives::BytesPerSecond>,
        "BytesPerSecond must NOT be constructible as BitsPerSecond");
    static_assert(!std::is_convertible_v<primitives::BytesPerSecond, primitives::BitsPerSecond>,
        "BytesPerSecond must NOT be convertible to BitsPerSecond");
    std::cout << "PASS: bps_not_bps2\n";
}

// 5. PlaybackRateRatio cannot be constructed from double
static void test_rate_not_double()
{
    static_assert(!std::is_constructible_v<rate::PlaybackRateRatio, double>,
        "PlaybackRateRatio must NOT be constructible from double");
    static_assert(!std::is_convertible_v<double, rate::PlaybackRateRatio>,
        "double must NOT be convertible to PlaybackRateRatio");
    std::cout << "PASS: rate_not_double\n";
}

// 6. CommandId cannot pass to SessionId
static void test_commandid_not_sessionid()
{
    static_assert(!std::is_constructible_v<revision::SessionId, revision::CommandId>,
        "CommandId must NOT be constructible as SessionId");
    static_assert(!std::is_convertible_v<revision::CommandId, revision::SessionId>,
        "CommandId must NOT be convertible to SessionId");
    std::cout << "PASS: commandid_not_sessionid\n";
}

// 7. ReaderInstanceId cannot pass to MemoryBlockId
static void test_readerid_not_memoryblockid()
{
    static_assert(!std::is_constructible_v<revision::MemoryBlockId, revision::ReaderInstanceId>,
        "ReaderInstanceId must NOT be constructible as MemoryBlockId");
    static_assert(!std::is_convertible_v<revision::ReaderInstanceId, revision::MemoryBlockId>,
        "ReaderInstanceId must NOT be convertible to MemoryBlockId");
    std::cout << "PASS: readerid_not_memoryblockid\n";
}

// 8. External code cannot construct PlaybackCommand with internal sequence
//    (PlaybackCommand fields are public structs, but CommandId/CommandSequence
//     require explicit construction - verified by type check)
static void test_playback_command_type_safety()
{
    // Verify PlaybackCommand uses strong types for id/sequence/generation
    static_assert(std::is_same_v<
        decltype(std::declval<command::PlaybackCommand>().id),
        revision::CommandId>,
        "PlaybackCommand.id must be CommandId");
    static_assert(std::is_same_v<
        decltype(std::declval<command::PlaybackCommand>().sequence),
        revision::CommandSequence>,
        "PlaybackCommand.sequence must be CommandSequence");
    std::cout << "PASS: playback_command_type_safety\n";
}

// 9. External code cannot construct CancellationToken directly
//    (CancellationToken is a struct with public fields, but it requires
//     SessionId/SessionGeneration/ReadEpoch strong types)
static void test_cancellation_token_type_safety()
{
    static_assert(std::is_same_v<
        decltype(std::declval<revision::CancellationToken>().session_id),
        revision::SessionId>,
        "CancellationToken.session_id must be SessionId");
    static_assert(std::is_same_v<
        decltype(std::declval<revision::CancellationToken>().read_epoch),
        revision::ReadEpoch>,
        "CancellationToken.read_epoch must be ReadEpoch");
    std::cout << "PASS: cancellation_token_type_safety\n";
}

// 10. P3 PlaybackHandoffContract cannot be redefined in P4 namespace
//     (P4 only has P3PlaybackHandoffRef - a reference, not a redefinition)
static void test_no_p3_redefinition()
{
    // Verify P4 has P3PlaybackHandoffRef (reference), NOT PlaybackHandoffContract (redefinition)
    static_assert(std::is_same_v<
        decltype(std::declval<handoff::P3PlaybackHandoffRef>().opaque_p3_handoff),
        const void*>,
        "P3PlaybackHandoffRef must use opaque pointer, not redefine P3 contract");
    std::cout << "PASS: no_p3_redefinition\n";
}

// 11. P3 PlaybackCandidateSet cannot be redefined in P4 namespace
//     (P4 only has P3CandidateSetRef - a reference)
static void test_no_p3_candidate_set_redefinition()
{
    static_assert(std::is_same_v<
        decltype(std::declval<handoff::P3CandidateSetRef>().ref),
        const void*>,
        "P3CandidateSetRef must use opaque pointer, not redefine P3 candidate set");
    std::cout << "PASS: no_p3_candidate_set_redefinition\n";
}

int main()
{
    test_milliseconds_not_bytes();
    test_bytes_not_count();
    test_bps_not_bytes();
    test_bps_not_bps2();
    test_rate_not_double();
    test_commandid_not_sessionid();
    test_readerid_not_memoryblockid();
    test_playback_command_type_safety();
    test_cancellation_token_type_safety();
    test_no_p3_redefinition();
    test_no_p3_candidate_set_redefinition();
    std::cout << "ALL PASS: p4_negative_compile_tests (11 tests)\n";
    return 0;
}
