// p6a_contract_tests.cpp — P6A contract compilation + behavior tests
// Tests that all 30 subdirectory headers compile and key types are usable.
#include <iostream>
#include <stdexcept>

// Foundation
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
#include "kivo/video/audio_plane/foundation/kivo_expected.hpp"
#include "kivo/video/audio_plane/foundation/kivo_optional.hpp"
#include "kivo/video/audio_plane/foundation/kivo_variant.hpp"

// All 30 subdirectories — compilation test
#include "kivo/video/audio_plane/boundary/p5_audio_packet_import_view.hpp"
#include "kivo/video/audio_plane/boundary/p5_audio_packet_import_result.hpp"
#include "kivo/video/audio_plane/boundary/p4_to_p6_audio_command.hpp"
#include "kivo/video/audio_plane/boundary/p6_recovery_seek_request.hpp"
#include "kivo/video/audio_plane/boundary/p6_to_p8_audio_summary.hpp"

#include "kivo/video/audio_plane/command/audio_command_id.hpp"
#include "kivo/video/audio_plane/command/audio_command_lifecycle.hpp"
#include "kivo/video/audio_plane/command/audio_command_ack.hpp"

#include "kivo/video/audio_plane/platform/audio_platform_backend_id.hpp"
#include "kivo/video/audio_plane/platform/audio_output_backend_contract.hpp"

#include "kivo/video/audio_plane/output/audio_output_handle.hpp"
#include "kivo/video/audio_plane/output/audio_output_mode.hpp"
#include "kivo/video/audio_plane/output/audio_output_capability_snapshot.hpp"

#include "kivo/video/audio_plane/packet/audio_packet_id.hpp"
#include "kivo/video/audio_plane/packet/audio_packet_import_view.hpp"
#include "kivo/video/audio_plane/packet/audio_packet_import_result.hpp"
#include "kivo/video/audio_plane/packet/audio_packet_pool_contract.hpp"

#include "kivo/video/audio_plane/data/audio_data_handle_id.hpp"
#include "kivo/video/audio_plane/data/audio_sync_byte_view.hpp"
#include "kivo/video/audio_plane/data/audio_owned_block_handle.hpp"
#include "kivo/video/audio_plane/data/audio_data_ownership_mode.hpp"

#include "kivo/video/audio_plane/decode/audio_decode_backend_contract.hpp"
#include "kivo/video/audio_plane/decode/audio_decode_backend_lifecycle.hpp"
#include "kivo/video/audio_plane/decode/decoded_audio_frame.hpp"

#include "kivo/video/audio_plane/passthrough/encoded_access_unit.hpp"
#include "kivo/video/audio_plane/passthrough/passthrough_claim_grade.hpp"

#include "kivo/video/audio_plane/frame/audio_frame_id.hpp"
#include "kivo/video/audio_plane/frame/audio_frame_pool_contract.hpp"

#include "kivo/video/audio_plane/render/audio_render_packet.hpp"
#include "kivo/video/audio_plane/render/audio_spsc_result.hpp"
#include "kivo/video/audio_plane/render/audio_spsc_queue_contract.hpp"
#include "kivo/video/audio_plane/render/audio_device_remainder_slot.hpp"

#include "kivo/video/audio_plane/generation/audio_engine_epoch.hpp"
#include "kivo/video/audio_plane/generation/audio_generation_wrap_policy.hpp"

#include "kivo/video/audio_plane/time/audio_pts.hpp"
#include "kivo/video/audio_plane/time/audio_checked_rescale.hpp"

#include "kivo/video/audio_plane/clock/audio_clock_candidate.hpp"
#include "kivo/video/audio_plane/clock/pcm_current_audible_time_formula.hpp"
#include "kivo/video/audio_plane/clock/total_output_latency_formula.hpp"

#include "kivo/video/audio_plane/sync/av_sync_policy.hpp"
#include "kivo/video/audio_plane/sync/audio_user_sync_offset_policy.hpp"

#include "kivo/video/audio_plane/seek/audio_seek_precision.hpp"

#include "kivo/video/audio_plane/gapless/audio_gapless_reserve_contract.hpp"

#include "kivo/video/audio_plane/format/audio_codec_id.hpp"
#include "kivo/video/audio_plane/format/audio_sample_format.hpp"
#include "kivo/video/audio_plane/format/audio_format_hash.hpp"

#include "kivo/video/audio_plane/conversion/audio_dither_policy.hpp"
#include "kivo/video/audio_plane/conversion/audio_sample_conversion_policy.hpp"

#include "kivo/video/audio_plane/channel/audio_canonical_speaker_order.hpp"
#include "kivo/video/audio_plane/channel/audio_channel_layout.hpp"

#include "kivo/video/audio_plane/mix/downmix_coefficient_table.hpp"
#include "kivo/video/audio_plane/mix/final_safety_limiter_policy.hpp"

#include "kivo/video/audio_plane/rate/playback_rate_policy.hpp"

#include "kivo/video/audio_plane/evidence/p6_evidence_record.hpp"
#include "kivo/video/audio_plane/evidence/audio_evidence_topology.hpp"

#include "kivo/video/audio_plane/capability/bitperfect_strict_claim_gate.hpp"
#include "kivo/video/audio_plane/capability/audio_claim_freshness_dependency_matrix.hpp"

#include "kivo/video/audio_plane/security/audio_pcm_dump_policy.hpp"
#include "kivo/video/audio_plane/security/audio_license_boundary.hpp"

#include "kivo/video/audio_plane/recovery/audio_failure_arbiter.hpp"
#include "kivo/video/audio_plane/recovery/audio_terminal_outcome.hpp"

#include "kivo/video/audio_plane/advanced_format/dsd_format_boundary.hpp"

#include "kivo/video/audio_plane/threading/audio_realtime_policy.hpp"
#include "kivo/video/audio_plane/threading/audio_allowed_wait_policy.hpp"

#include "kivo/video/audio_plane/test_contract/audio_benchmark_slo.hpp"

#include "kivo/video/audio_plane/terminology/audio_term_glossary.hpp"

#include "kivo/video/audio_plane/version/p6_boundary_contract_version.hpp"

namespace kap = kivo::video::audio_plane;

int main() {
    int passed = 0;
    int total = 0;

    // Foundation
    total++; { kap::ContractMetadata m{"test", "test.hpp", "Contract", 1}; if (m.version == 1) { passed++; } }
    total++; { kivo::Expected<int, std::string> ok(42); if (ok.has_value() && ok.value() == 42) { passed++; } }
    total++; { kivo::Expected<int, std::string> err(std::string("fail")); if (err.is_error() && err.error() == "fail") { passed++; } }

    // boundary
    total++; { kap::packet::AudioPacketImportView v{}; v.pts = 1000; v.is_key_packet = true; if (v.pts == 1000) { passed++; } }
    total++; { using R = kap::packet::AudioPacketImportResult; if (R::AcceptedMoved != R::RejectedOversize) { passed++; } }

    // command
    total++; { kap::command::AudioCommandId id{}; if (!id.is_valid()) { passed++; } }
    total++; { using S = kap::command::AudioCommandLifecycleState; kap::command::AudioCommandLifecycle lc{}; lc.state = S::Executed; if (lc.is_terminal()) { passed++; } }

    // output
    total++; { using M = kap::output::AudioOutputMode; if (M::Unknown != M::PcmExclusiveExact) { passed++; } }
    total++; { kap::output::AudioOutputCapabilitySnapshot snap{}; if (snap.unknown_blocks_positive_claim()) { passed++; } }

    // data
    total++; { using O = kap::data::AudioDataOwnershipMode; if (O::SyncBorrowedByteView != O::Rejected) { passed++; } }

    // decode
    total++; { using S = kap::decode::AudioDecodeBackendState; kap::decode::AudioDecodeBackendLifecycle lc{}; lc.state = S::TimedOut; if (lc.is_timed_out()) { passed++; } }

    // render
    total++; { static_assert(std::is_nothrow_move_constructible_v<kap::render::AudioRenderPacket>); static_assert(std::is_nothrow_destructible_v<kap::render::AudioRenderPacket>); kap::render::AudioRenderPacket p{}; if (p.sample_count == 0) { passed++; } }
    total++; { using P = kap::render::AudioSpscPushResult; if (P::Pushed != P::Full) { passed++; } }

    // generation
    total++; { kap::generation::AudioEngineEpoch e{}; if (!e.is_valid()) { passed++; } }

    // capability
    total++; { kap::capability::BitperfectStrictClaimGate gate{}; if (!gate.all_conditions_met()) { passed++; } }

    // recovery
    total++; { using O = kap::recovery::AudioTerminalOutcome; if (O::Recovered != O::ProcessSafetyExceeded) { passed++; } }

    // format
    total++; { kap::format::AudioFormatHash h{}; if (h.is_accelerator_not_sole_truth) { passed++; } }

    // version
    total++; { kap::version::P6BoundaryContractVersion v{}; if (v.major == 2) { passed++; } }

    // mix
    total++; { kap::mix::DownmixCoefficientTable t{}; if (t.center_gain > 0.7 && t.center_gain < 0.71) { passed++; } }

    // rate
    total++; { kap::rate::PlaybackRatePolicy p{}; if (p.min_rate == 0.25 && p.max_rate == 4.0) { passed++; } }

    std::cout << "=== P6A Contract Tests ===" << "\n";
    std::cout << "Passed: " << passed << "/" << total << "\n";
    if (passed == total) {
        std::cout << "=== ALL PASS ===" << "\n";
        return 0;
    } else {
        std::cout << "=== SOME FAILED ===" << "\n";
        return 1;
    }
}
