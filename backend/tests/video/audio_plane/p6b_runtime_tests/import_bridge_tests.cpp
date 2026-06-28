// import_bridge_tests.cpp — P6B import_bridge runtime tests (B1-B4)
// Tests FakeP5PacketImporter, PacketImportValidator, PacketOwnershipTransfer,
// and PacketSequenceChecker against their header contracts.
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <stdexcept>

#include "kivo/video/audio_plane/runtime/import_bridge/fake_p5_packet_importer.hpp"
#include "kivo/video/audio_plane/runtime/import_bridge/packet_import_validator.hpp"
#include "kivo/video/audio_plane/runtime/import_bridge/packet_ownership_transfer.hpp"
#include "kivo/video/audio_plane/runtime/import_bridge/packet_sequence_checker.hpp"

namespace ib = kivo::video::audio_plane::runtime::import_bridge;
using namespace kivo::video::audio_plane;

static int g_failed = 0;

#define P6B_RUN(name) do { try { name(); printf(" PASS: %s\n", #name); } catch(...) { ++g_failed; printf(" FAIL: %s\n", #name); } } while(0)

// ---------- B1: FakeP5PacketImporter ----------

static void test_b01_importer_default_not_initialized() {
    ib::FakeP5PacketImporter imp;
    assert(imp.state() == ib::ImporterState::NotInitialized);
}

static void test_b02_importer_initialize_transitions_to_ready() {
    ib::FakeP5PacketImporter imp;
    imp.initialize();
    assert(imp.state() == ib::ImporterState::Ready);
}

static void test_b03_importer_rejects_before_initialize() {
    ib::FakeP5PacketImporter imp;
    boundary::P5AudioPacketImportView v{};
    v.stream_id = 1; v.track_id = 1; v.packet_id = 10;
    auto r = imp.import(v, data::AudioDataOwnershipMode::SyncBorrowedByteView);
    assert(r.is_error());
    assert(r.error() == ib::ImportError::NotInitialized);
}

static void test_b04_import_sync_borrowed_byte_view_accepted_copied() {
    ib::FakeP5PacketImporter imp;
    imp.initialize();
    boundary::P5AudioPacketImportView v{};
    v.stream_id = 1; v.track_id = 2; v.packet_id = 100;
    auto r = imp.import(v, data::AudioDataOwnershipMode::SyncBorrowedByteView);
    assert(r.has_value());
    assert(r.value() == packet::AudioPacketImportResult::AcceptedCopied);
    assert(imp.copied_count() == 1);
    assert(imp.moved_count() == 0);
    assert(imp.pool_slot_count() == 0);
    assert(imp.last_packet_id().value == 100);
}

static void test_b05_import_move_owned_block_accepted_moved() {
    ib::FakeP5PacketImporter imp;
    imp.initialize();
    boundary::P5AudioPacketImportView v{};
    v.stream_id = 1; v.track_id = 2; v.packet_id = 101;
    auto r = imp.import(v, data::AudioDataOwnershipMode::MoveOwnedBlock);
    assert(r.has_value());
    assert(r.value() == packet::AudioPacketImportResult::AcceptedMoved);
    assert(imp.moved_count() == 1);
}

static void test_b06_import_pool_slot_owned_by_p6_accepted_pool_slot() {
    ib::FakeP5PacketImporter imp;
    imp.initialize();
    boundary::P5AudioPacketImportView v{};
    v.stream_id = 1; v.track_id = 2; v.packet_id = 102;
    auto r = imp.import(v, data::AudioDataOwnershipMode::PoolSlotOwnedByP6);
    assert(r.has_value());
    assert(r.value() == packet::AudioPacketImportResult::AcceptedPoolSlot);
    assert(imp.pool_slot_count() == 1);
}

static void test_b07_import_copied_into_p6_packet_pool_accepted_copied() {
    ib::FakeP5PacketImporter imp;
    imp.initialize();
    boundary::P5AudioPacketImportView v{};
    v.stream_id = 1; v.track_id = 2; v.packet_id = 103;
    auto r = imp.import(v, data::AudioDataOwnershipMode::CopiedIntoP6PacketPool);
    assert(r.has_value());
    assert(r.value() == packet::AudioPacketImportResult::AcceptedCopied);
    assert(imp.copied_count() == 1);
}

static void test_b08_import_external_opaque_invalid_ownership_mode() {
    ib::FakeP5PacketImporter imp;
    imp.initialize();
    boundary::P5AudioPacketImportView v{};
    v.stream_id = 1; v.track_id = 2; v.packet_id = 104;
    auto r = imp.import(v, data::AudioDataOwnershipMode::ExternalOpaque);
    assert(r.is_error());
    assert(r.error() == ib::ImportError::InvalidOwnershipMode);
}

static void test_b09_import_rejected_mode_returns_rejected_invalid_ownership() {
    ib::FakeP5PacketImporter imp;
    imp.initialize();
    boundary::P5AudioPacketImportView v{};
    v.stream_id = 1; v.track_id = 2; v.packet_id = 105;
    auto r = imp.import(v, data::AudioDataOwnershipMode::Rejected);
    assert(r.has_value());
    assert(r.value() == packet::AudioPacketImportResult::RejectedInvalidOwnership);
}

static void test_b10_import_rejects_zero_stream_id() {
    ib::FakeP5PacketImporter imp;
    imp.initialize();
    boundary::P5AudioPacketImportView v{};
    v.stream_id = 0; v.track_id = 2; v.packet_id = 106;
    auto r = imp.import(v, data::AudioDataOwnershipMode::SyncBorrowedByteView);
    assert(r.has_value());
    assert(r.value() == packet::AudioPacketImportResult::RejectedInvalidGeneration);
}

static void test_b11_import_rejects_zero_track_id() {
    ib::FakeP5PacketImporter imp;
    imp.initialize();
    boundary::P5AudioPacketImportView v{};
    v.stream_id = 1; v.track_id = 0; v.packet_id = 107;
    auto r = imp.import(v, data::AudioDataOwnershipMode::SyncBorrowedByteView);
    assert(r.has_value());
    assert(r.value() == packet::AudioPacketImportResult::RejectedInvalidGeneration);
}

static void test_b12_import_rejects_protected_packet() {
    ib::FakeP5PacketImporter imp;
    imp.initialize();
    boundary::P5AudioPacketImportView v{};
    v.stream_id = 1; v.track_id = 2; v.packet_id = 108;
    v.encrypted_or_protected = true;
    auto r = imp.import(v, data::AudioDataOwnershipMode::SyncBorrowedByteView);
    assert(r.has_value());
    assert(r.value() == packet::AudioPacketImportResult::RejectedProtectedPolicy);
}

static void test_b13_import_transitions_state_to_importing() {
    ib::FakeP5PacketImporter imp;
    imp.initialize();
    boundary::P5AudioPacketImportView v{};
    v.stream_id = 1; v.track_id = 2; v.packet_id = 109;
    (void)imp.import(v, data::AudioDataOwnershipMode::SyncBorrowedByteView);
    assert(imp.state() == ib::ImporterState::Importing);
}

static void test_b14_import_after_begin_drain_rejects_new_packets() {
    ib::FakeP5PacketImporter imp;
    imp.initialize();
    boundary::P5AudioPacketImportView v{};
    v.stream_id = 1; v.track_id = 2; v.packet_id = 110;
    (void)imp.import(v, data::AudioDataOwnershipMode::SyncBorrowedByteView);
    imp.begin_drain();
    assert(imp.state() == ib::ImporterState::Draining);
    boundary::P5AudioPacketImportView v2{};
    v2.stream_id = 1; v2.track_id = 2; v2.packet_id = 111;
    auto r = imp.import(v2, data::AudioDataOwnershipMode::SyncBorrowedByteView);
    assert(r.is_error());
    assert(r.error() == ib::ImportError::DrainingRejectsNewPackets);
}

static void test_b15_import_after_close_returns_closed() {
    ib::FakeP5PacketImporter imp;
    imp.initialize();
    imp.close();
    assert(imp.state() == ib::ImporterState::Closed);
    boundary::P5AudioPacketImportView v{};
    v.stream_id = 1; v.track_id = 2; v.packet_id = 112;
    auto r = imp.import(v, data::AudioDataOwnershipMode::SyncBorrowedByteView);
    assert(r.is_error());
    assert(r.error() == ib::ImportError::Closed);
}

// ---------- B2: PacketImportValidator ----------

static void test_b16_validator_stream_id_track_id_valid() {
    ib::PacketImportValidator val;
    boundary::P5AudioPacketImportView v{};
    v.stream_id = 1; v.track_id = 2; v.engine_epoch = 3; v.pipeline_generation = 4;
    assert(val.validate_stream_id_track_id_generation_epoch(v));
}

static void test_b17_validator_stream_id_track_id_invalid_zero_stream() {
    ib::PacketImportValidator val;
    boundary::P5AudioPacketImportView v{};
    v.stream_id = 0; v.track_id = 2; v.engine_epoch = 3; v.pipeline_generation = 4;
    assert(!val.validate_stream_id_track_id_generation_epoch(v));
}

static void test_b18_validator_stream_id_track_id_invalid_zero_track() {
    ib::PacketImportValidator val;
    boundary::P5AudioPacketImportView v{};
    v.stream_id = 1; v.track_id = 0; v.engine_epoch = 3; v.pipeline_generation = 4;
    assert(!val.validate_stream_id_track_id_generation_epoch(v));
}

static void test_b19_validator_stream_id_track_id_invalid_zero_epoch() {
    ib::PacketImportValidator val;
    boundary::P5AudioPacketImportView v{};
    v.stream_id = 1; v.track_id = 2; v.engine_epoch = 0; v.pipeline_generation = 4;
    assert(!val.validate_stream_id_track_id_generation_epoch(v));
}

static void test_b20_validator_stream_id_track_id_invalid_zero_generation() {
    ib::PacketImportValidator val;
    boundary::P5AudioPacketImportView v{};
    v.stream_id = 1; v.track_id = 2; v.engine_epoch = 3; v.pipeline_generation = 0;
    assert(!val.validate_stream_id_track_id_generation_epoch(v));
}

static void test_b21_validator_sequence_monotonic_ok() {
    ib::PacketImportValidator val;
    boundary::P5AudioPacketImportView v{};
    v.packet_sequence = 10;
    v.is_discontinuity = false;
    assert(val.validate_packet_sequence_monotonicity(v, 9));
}

static void test_b22_validator_sequence_non_monotonic_without_discontinuity() {
    ib::PacketImportValidator val;
    boundary::P5AudioPacketImportView v{};
    v.packet_sequence = 5;
    v.is_discontinuity = false;
    assert(!val.validate_packet_sequence_monotonicity(v, 9));
}

static void test_b23_validator_sequence_non_monotonic_with_discontinuity_allowed() {
    ib::PacketImportValidator val;
    boundary::P5AudioPacketImportView v{};
    v.packet_sequence = 5;
    v.is_discontinuity = true;
    assert(val.validate_packet_sequence_monotonicity(v, 9));
}

static void test_b24_validator_sequence_equal_rejected() {
    ib::PacketImportValidator val;
    boundary::P5AudioPacketImportView v{};
    v.packet_sequence = 9;
    v.is_discontinuity = false;
    assert(!val.validate_packet_sequence_monotonicity(v, 9));
}

static void test_b25_validator_codec_private_data_key_packet_requires_ref() {
    ib::PacketImportValidator val;
    boundary::P5AudioPacketImportView v{};
    v.is_key_packet = true;
    v.codec_private_data_ref = 0;
    assert(!val.validate_codec_private_data_before_decode_init(v));
}

static void test_b26_validator_codec_private_data_key_packet_with_ref_ok() {
    ib::PacketImportValidator val;
    boundary::P5AudioPacketImportView v{};
    v.is_key_packet = true;
    v.codec_private_data_ref = 42;
    assert(val.validate_codec_private_data_before_decode_init(v));
}

static void test_b27_validator_codec_private_data_non_key_packet_ok_without_ref() {
    ib::PacketImportValidator val;
    boundary::P5AudioPacketImportView v{};
    v.is_key_packet = false;
    v.codec_private_data_ref = 0;
    assert(val.validate_codec_private_data_before_decode_init(v));
}

static void test_b28_validator_protected_summary_redacted_ok() {
    ib::PacketImportValidator val;
    boundary::P5AudioPacketImportView v{};
    v.encrypted_or_protected = true;
    v.side_data_summary = 0;
    v.codec_private_data_ref = 0;
    assert(val.protected_encrypted_summary_redacted(v));
}

static void test_b29_validator_protected_summary_not_redacted_side_data() {
    ib::PacketImportValidator val;
    boundary::P5AudioPacketImportView v{};
    v.encrypted_or_protected = true;
    v.side_data_summary = 7;
    v.codec_private_data_ref = 0;
    assert(!val.protected_encrypted_summary_redacted(v));
}

static void test_b30_validator_protected_summary_not_redacted_codec_ref() {
    ib::PacketImportValidator val;
    boundary::P5AudioPacketImportView v{};
    v.encrypted_or_protected = true;
    v.side_data_summary = 0;
    v.codec_private_data_ref = 9;
    assert(!val.protected_encrypted_summary_redacted(v));
}

static void test_b31_validator_protected_summary_not_applicable_when_unprotected() {
    ib::PacketImportValidator val;
    boundary::P5AudioPacketImportView v{};
    v.encrypted_or_protected = false;
    v.side_data_summary = 7;
    v.codec_private_data_ref = 9;
    assert(val.protected_encrypted_summary_redacted(v));
}

static void test_b32_validator_validate_all_passes_clean_view() {
    ib::PacketImportValidator val;
    boundary::P5AudioPacketImportView v{};
    v.stream_id = 1; v.track_id = 2; v.engine_epoch = 3; v.pipeline_generation = 4;
    v.packet_sequence = 10; v.is_discontinuity = false;
    v.is_key_packet = true; v.codec_private_data_ref = 42;
    v.encrypted_or_protected = false;
    packet::P5AudioPacketImportContract contract{};
    assert(val.validate_all(v, contract, 9));
}

static void test_b33_validator_validate_all_fails_on_rule_one() {
    ib::PacketImportValidator val;
    boundary::P5AudioPacketImportView v{};
    v.stream_id = 0;  // invalid
    v.track_id = 2; v.engine_epoch = 3; v.pipeline_generation = 4;
    packet::P5AudioPacketImportContract contract{};
    assert(!val.validate_all(v, contract, 0));
}

// ---------- B3: PacketOwnershipTransfer ----------

static void test_b34_transfer_default_p5_owned() {
    ib::PacketOwnershipTransfer t;
    assert(t.state() == ib::TransferState::P5Owned);
}

static void test_b35_transfer_begin_transitions_to_transferring() {
    ib::PacketOwnershipTransfer t;
    packet::AudioPacketId pid{123};
    auto r = t.begin_transfer(pid, data::AudioDataOwnershipMode::MoveOwnedBlock);
    assert(r.has_value());
    assert(t.state() == ib::TransferState::Transferring);
    assert(t.packet_id().value == 123);
    assert(t.mode() == data::AudioDataOwnershipMode::MoveOwnedBlock);
}

static void test_b36_transfer_begin_rejects_when_not_p5_owned() {
    ib::PacketOwnershipTransfer t;
    packet::AudioPacketId pid{1};
    (void)t.begin_transfer(pid, data::AudioDataOwnershipMode::MoveOwnedBlock);
    auto r = t.begin_transfer(pid, data::AudioDataOwnershipMode::MoveOwnedBlock);
    assert(r.is_error());
    assert(r.error() == ib::TransferError::NotInP5OwnedState);
}

static void test_b37_transfer_complete_transitions_to_p6_owned() {
    ib::PacketOwnershipTransfer t;
    packet::AudioPacketId pid{1};
    (void)t.begin_transfer(pid, data::AudioDataOwnershipMode::MoveOwnedBlock);
    auto r = t.complete_transfer();
    assert(r.has_value());
    assert(t.state() == ib::TransferState::P6Owned);
}

static void test_b38_transfer_complete_rejects_when_not_transferring() {
    ib::PacketOwnershipTransfer t;
    auto r = t.complete_transfer();
    assert(r.is_error());
    assert(r.error() == ib::TransferError::NotInTransferringState);
}

static void test_b39_transfer_reject_from_p5_owned_lands_in_released() {
    ib::PacketOwnershipTransfer t;
    t.reject(ib::TransferRejectReason::ValidationFailed);
    assert(t.state() == ib::TransferState::Released);
}

static void test_b40_transfer_reject_from_transferring_lands_in_released() {
    ib::PacketOwnershipTransfer t;
    packet::AudioPacketId pid{1};
    (void)t.begin_transfer(pid, data::AudioDataOwnershipMode::MoveOwnedBlock);
    t.reject(ib::TransferRejectReason::ProtectedPolicy);
    assert(t.state() == ib::TransferState::Released);
}

static void test_b41_transfer_supersede_from_p6_owned_lands_in_released() {
    ib::PacketOwnershipTransfer t;
    packet::AudioPacketId pid{1};
    (void)t.begin_transfer(pid, data::AudioDataOwnershipMode::MoveOwnedBlock);
    (void)t.complete_transfer();
    t.supersede();
    assert(t.state() == ib::TransferState::Released);
}

static void test_b42_transfer_supersede_noop_from_p5_owned() {
    ib::PacketOwnershipTransfer t;
    t.supersede();
    assert(t.state() == ib::TransferState::P5Owned);
}

static void test_b43_transfer_release_from_p6_owned_lands_in_released() {
    ib::PacketOwnershipTransfer t;
    packet::AudioPacketId pid{1};
    (void)t.begin_transfer(pid, data::AudioDataOwnershipMode::MoveOwnedBlock);
    (void)t.complete_transfer();
    auto r = t.release();
    assert(r.has_value());
    assert(t.state() == ib::TransferState::Released);
}

static void test_b44_transfer_release_already_released() {
    ib::PacketOwnershipTransfer t;
    packet::AudioPacketId pid{1};
    (void)t.begin_transfer(pid, data::AudioDataOwnershipMode::MoveOwnedBlock);
    (void)t.complete_transfer();
    (void)t.release();
    auto r = t.release();
    assert(r.is_error());
    assert(r.error() == ib::TransferError::AlreadyReleased);
}

static void test_b45_transfer_release_rejects_when_not_p6_owned() {
    ib::PacketOwnershipTransfer t;
    auto r = t.release();
    assert(r.is_error());
    assert(r.error() == ib::TransferError::NotInTransferringState);
}

static void test_b46_transfer_full_lifecycle_p5_to_p6_to_released() {
    ib::PacketOwnershipTransfer t;
    packet::AudioPacketId pid{555};
    auto b = t.begin_transfer(pid, data::AudioDataOwnershipMode::PoolSlotOwnedByP6);
    assert(b.has_value());
    auto c = t.complete_transfer();
    assert(c.has_value());
    auto r = t.release();
    assert(r.has_value());
    assert(t.state() == ib::TransferState::Released);
}

// ---------- B4: PacketSequenceChecker ----------

static void test_b47_sequence_checker_monotonic_ok() {
    ib::PacketSequenceChecker chk;
    packet::AudioPacketSequence cur{10};
    packet::AudioPacketSequence prev{9};
    auto r = chk.check(cur, false, prev);
    assert(r == ib::SequenceCheckResult::Monotonic);
}

static void test_b48_sequence_checker_duplicate() {
    ib::PacketSequenceChecker chk;
    packet::AudioPacketSequence cur{9};
    packet::AudioPacketSequence prev{9};
    auto r = chk.check(cur, false, prev);
    assert(r == ib::SequenceCheckResult::Duplicate);
}

static void test_b49_sequence_checker_non_monotonic_with_discontinuity_accepted() {
    ib::PacketSequenceChecker chk;
    packet::AudioPacketSequence cur{5};
    packet::AudioPacketSequence prev{9};
    auto r = chk.check(cur, true, prev);
    assert(r == ib::SequenceCheckResult::NonMonotonicWithDiscontinuity);
}

static void test_b50_sequence_checker_non_monotonic_without_discontinuity_violation() {
    ib::PacketSequenceChecker chk;
    packet::AudioPacketSequence cur{5};
    packet::AudioPacketSequence prev{9};
    auto r = chk.check(cur, false, prev);
    assert(r == ib::SequenceCheckResult::NonMonotonicViolation);
}

static void test_b51_sequence_checker_timestamp_ok_forward() {
    ib::PacketSequenceChecker chk;
    auto r = chk.check_timestamp(1000, 900, true);
    assert(r == ib::TimestampCheckResult::Ok);
}

static void test_b52_sequence_checker_timestamp_ok_equal() {
    ib::PacketSequenceChecker chk;
    auto r = chk.check_timestamp(900, 900, true);
    assert(r == ib::TimestampCheckResult::Ok);
}

static void test_b53_sequence_checker_timestamp_backward_jump() {
    ib::PacketSequenceChecker chk;
    // small backward delta with monotonic sequence -> BackwardJump
    auto r = chk.check_timestamp(800, 900, true);
    assert(r == ib::TimestampCheckResult::BackwardJump);
}

static void test_b54_sequence_checker_timestamp_wrap_on_large_delta() {
    ib::PacketSequenceChecker chk;
    // very large backward delta -> Wrap
    constexpr int64_t big = (static_cast<int64_t>(1) << 62) + 1;
    auto r = chk.check_timestamp(0, big, true);
    assert(r == ib::TimestampCheckResult::Wrap);
}

static void test_b55_sequence_checker_timestamp_backward_with_non_monotonic_sequence_wraps() {
    ib::PacketSequenceChecker chk;
    // small backward delta but non-monotonic sequence -> treated as Wrap
    auto r = chk.check_timestamp(800, 900, false);
    assert(r == ib::TimestampCheckResult::Wrap);
}

int main() {
    printf("=== P6B import_bridge_tests ===\n");

    // B1: FakeP5PacketImporter
    P6B_RUN(test_b01_importer_default_not_initialized);
    P6B_RUN(test_b02_importer_initialize_transitions_to_ready);
    P6B_RUN(test_b03_importer_rejects_before_initialize);
    P6B_RUN(test_b04_import_sync_borrowed_byte_view_accepted_copied);
    P6B_RUN(test_b05_import_move_owned_block_accepted_moved);
    P6B_RUN(test_b06_import_pool_slot_owned_by_p6_accepted_pool_slot);
    P6B_RUN(test_b07_import_copied_into_p6_packet_pool_accepted_copied);
    P6B_RUN(test_b08_import_external_opaque_invalid_ownership_mode);
    P6B_RUN(test_b09_import_rejected_mode_returns_rejected_invalid_ownership);
    P6B_RUN(test_b10_import_rejects_zero_stream_id);
    P6B_RUN(test_b11_import_rejects_zero_track_id);
    P6B_RUN(test_b12_import_rejects_protected_packet);
    P6B_RUN(test_b13_import_transitions_state_to_importing);
    P6B_RUN(test_b14_import_after_begin_drain_rejects_new_packets);
    P6B_RUN(test_b15_import_after_close_returns_closed);

    // B2: PacketImportValidator
    P6B_RUN(test_b16_validator_stream_id_track_id_valid);
    P6B_RUN(test_b17_validator_stream_id_track_id_invalid_zero_stream);
    P6B_RUN(test_b18_validator_stream_id_track_id_invalid_zero_track);
    P6B_RUN(test_b19_validator_stream_id_track_id_invalid_zero_epoch);
    P6B_RUN(test_b20_validator_stream_id_track_id_invalid_zero_generation);
    P6B_RUN(test_b21_validator_sequence_monotonic_ok);
    P6B_RUN(test_b22_validator_sequence_non_monotonic_without_discontinuity);
    P6B_RUN(test_b23_validator_sequence_non_monotonic_with_discontinuity_allowed);
    P6B_RUN(test_b24_validator_sequence_equal_rejected);
    P6B_RUN(test_b25_validator_codec_private_data_key_packet_requires_ref);
    P6B_RUN(test_b26_validator_codec_private_data_key_packet_with_ref_ok);
    P6B_RUN(test_b27_validator_codec_private_data_non_key_packet_ok_without_ref);
    P6B_RUN(test_b28_validator_protected_summary_redacted_ok);
    P6B_RUN(test_b29_validator_protected_summary_not_redacted_side_data);
    P6B_RUN(test_b30_validator_protected_summary_not_redacted_codec_ref);
    P6B_RUN(test_b31_validator_protected_summary_not_applicable_when_unprotected);
    P6B_RUN(test_b32_validator_validate_all_passes_clean_view);
    P6B_RUN(test_b33_validator_validate_all_fails_on_rule_one);

    // B3: PacketOwnershipTransfer
    P6B_RUN(test_b34_transfer_default_p5_owned);
    P6B_RUN(test_b35_transfer_begin_transitions_to_transferring);
    P6B_RUN(test_b36_transfer_begin_rejects_when_not_p5_owned);
    P6B_RUN(test_b37_transfer_complete_transitions_to_p6_owned);
    P6B_RUN(test_b38_transfer_complete_rejects_when_not_transferring);
    P6B_RUN(test_b39_transfer_reject_from_p5_owned_lands_in_released);
    P6B_RUN(test_b40_transfer_reject_from_transferring_lands_in_released);
    P6B_RUN(test_b41_transfer_supersede_from_p6_owned_lands_in_released);
    P6B_RUN(test_b42_transfer_supersede_noop_from_p5_owned);
    P6B_RUN(test_b43_transfer_release_from_p6_owned_lands_in_released);
    P6B_RUN(test_b44_transfer_release_already_released);
    P6B_RUN(test_b45_transfer_release_rejects_when_not_p6_owned);
    P6B_RUN(test_b46_transfer_full_lifecycle_p5_to_p6_to_released);

    // B4: PacketSequenceChecker
    P6B_RUN(test_b47_sequence_checker_monotonic_ok);
    P6B_RUN(test_b48_sequence_checker_duplicate);
    P6B_RUN(test_b49_sequence_checker_non_monotonic_with_discontinuity_accepted);
    P6B_RUN(test_b50_sequence_checker_non_monotonic_without_discontinuity_violation);
    P6B_RUN(test_b51_sequence_checker_timestamp_ok_forward);
    P6B_RUN(test_b52_sequence_checker_timestamp_ok_equal);
    P6B_RUN(test_b53_sequence_checker_timestamp_backward_jump);
    P6B_RUN(test_b54_sequence_checker_timestamp_wrap_on_large_delta);
    P6B_RUN(test_b55_sequence_checker_timestamp_backward_with_non_monotonic_sequence_wraps);

    printf("=== P6B import_bridge_tests: %d failed ===\n", g_failed);
    return g_failed == 0 ? 0 : 1;
}
