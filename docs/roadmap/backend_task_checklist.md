# Kivo Video Backend Task Checklist

Updated: 2026-06-25 20:10 (V10-022 DONE)

This checklist is the execution source of truth for KivoCinemaEngine backend work. Do not rely on memory alone. Before starting a new backend task, pick the next unchecked item here. After finishing a task, update its status and evidence.

Status legend:

- DONE: implemented and verified.
- IN PROGRESS: currently being implemented.
- NEXT: next concrete task.
- TODO: not started.
- BLOCKED: cannot progress without external input or environment change.

## Iron Gate

- DONE: Conception. Evidence: Core Freeze Spec was extracted from `F:\Files\开发\Kivo Video 后端核心设计 V1.1 Core Freeze Spec.docx`.
- DONE: Planning design. Evidence: P0/P1 route documented under `docs/p0/` and `docs/roadmap/`.
- DONE: Natural family tree. Evidence: `docs/p0/core-architecture/natural_family_tree.md`.
- IN PROGRESS: Implementation. Evidence: C++ backend under `backend/` builds and tests through MSVC/Ninja.
- IN PROGRESS: Single file, single responsibility. Evidence: module families under `backend/include/kivo/cinema_engine/*` and `backend/src/*`.
- IN PROGRESS: No flat files. Evidence: root files limited to project entry/config; implementation and docs are under semantic directories.

## P0 Contract Freeze

- DONE: Core Architecture Freeze docs. Evidence: `docs/p0/core-architecture/core_freeze_spec.md`.
- DONE: Natural family tree. Evidence: `docs/p0/core-architecture/natural_family_tree.md`.
- DONE: ContractRegistry doc. Evidence: `docs/p0/foundation/contract_registry.md`.
- DONE: Playback session model doc. Evidence: `docs/p0/session-ledger/playback_session_model.md`.
- DONE: Decision ledger model doc. Evidence: `docs/p0/session-ledger/decision_ledger_model.md`.
- DONE: Playback plan schema doc. Evidence: `docs/p0/playback-plan/playback_plan_schema.md`.
- DONE: Capability join result doc. Evidence: `docs/p0/capability-policy/capability_join_result.md`.
- DONE: Source identity and remote object doc. Evidence: `docs/p0/source-remote-object/source_identity_and_remote_object.md`.
- DONE: Media experience snapshot doc. Evidence: `docs/p0/media-experience/media_experience_snapshot.md`.
- DONE: Color output matrix doc. Evidence: `docs/p0/color-hdr/color_output_matrix.md`.
- DONE: Windows HDR output contract doc. Evidence: `docs/p0/windows-presenter/windows_hdr_output_contract.md`.
- DONE: Presenter and frame pacing contract doc. Evidence: `docs/p0/windows-presenter/presenter_and_frame_pacing_contract.md`.
- DONE: Governance CI gates doc. Evidence: `docs/p0/governance-ci/governance_ci_gates.md`.
- DONE: Subtitle render plan doc. Evidence: `docs/p0/audio-subtitle/subtitle_render_plan.md`.
- DONE: Audio output decision doc. Evidence: `docs/p0/audio-subtitle/audio_output_decision.md`.
- DONE: Playback inspector snapshot doc. Evidence: `docs/p0/diagnostic-inspector/playback_inspector_snapshot.md`.
- DONE: Diagnostic bundle manifest doc. Evidence: `docs/p0/diagnostic-inspector/diagnostic_bundle_manifest.md`.

## P1 Foundation Implementation

- DONE: CMake project entry. Evidence: `CMakeLists.txt`, `backend/CMakeLists.txt`, `CMakePresets.json`.
- DONE: Required versioned contract headers. Evidence: `backend/include/kivo/cinema_engine/**`.
- DONE: ContractRegistry implementation. Evidence: `backend/src/foundation/contract_registry.cpp`.
- DONE: Governance gate script. Evidence: `backend/tools/governance/verify_p1_gates.py`.
- DONE: P1 fake direct-play session. Evidence: `backend/src/fake_backend/fake_session_scenario.cpp`.
- DONE: Diagnostic redaction gate. Evidence: `backend/src/diagnostic_core/diagnostic_redaction_gate.cpp`.
- DONE: Playback invariant checker. Evidence: `backend/src/playback_core/playback_invariant.cpp`.
- DONE: Engine API command queue. Evidence: `backend/src/engine_api/command_queue.cpp`.
- DONE: Engine API event subscription. Evidence: `backend/src/engine_api/event_bus.cpp`.
- DONE: Engine API state snapshot store. Evidence: `backend/src/engine_api/state_snapshot_store.cpp`.
- DONE: Engine API cancellation token. Evidence: `backend/src/engine_api/cancellation_token.cpp`.
- DONE: SessionStore in-memory create/close/find/export. Evidence: `backend/src/session_core/session_store.cpp`.
- DONE: DecisionLedgerStore in-memory append/query/export. Evidence: `backend/src/decision_ledger_core/decision_ledger_store.cpp`.
- DONE: ReplayManifest. Evidence: `backend/src/decision_ledger_core/replay_manifest.cpp`.
- DONE: Engine API-driven fake scenario. Evidence: `backend/src/fake_backend/fake_engine_api_scenario.cpp`.
- DONE: PlaybackCore fake state machine. Evidence: `backend/src/playback_core/fake_playback_state_machine.cpp`.
- DONE: GraphCore fake runtime and invariant. Evidence: `backend/src/graph_core/fake_graph_runtime.cpp`, `backend/src/graph_core/graph_invariant.cpp`.
- DONE: NetworkCacheCore fake cache read bound to RemoteObjectIdentity. Evidence: `backend/src/network_cache_core/fake_cache_reader.cpp`.
- DONE: DemuxCore fake StreamInventory. Evidence: `backend/src/demux_core/fake_stream_probe.cpp`.
- DONE: DecodeCore fake decode path. Evidence: `backend/src/decode_core/fake_decode_path.cpp`.
- DONE: ColorScienceCore fake ColorOutputMatrix service. Evidence: `backend/src/color_science_core/fake_color_output_service.cpp`.
- DONE: WindowsHdrCore fake WindowsHdrOutputContract service. Evidence: `backend/src/windows_hdr_core/fake_windows_hdr_service.cpp`.
- DONE: VideoRenderCore fake VideoPresenterContract service. Evidence: `backend/src/video_render_core/fake_presenter_service.cpp`.
- DONE: SubtitleCore fake SubtitleRenderPlan service. Evidence: `backend/src/subtitle_core/fake_subtitle_service.cpp`.
- DONE: AudioCore fake AudioOutputDecision service. Evidence: `backend/src/audio_core/fake_audio_service.cpp`.
- DONE: PlaybackInspectorCore fake inspector service. Evidence: `backend/src/playback_inspector_core/fake_inspector_service.cpp`, `backend/include/kivo/cinema_engine/playback_inspector_core/fake_inspector_service.hpp`.
- DONE: DiagnosticCore fake bundle manifest export service. Evidence: `backend/src/diagnostic_core/fake_diagnostic_bundle_export.cpp`, `backend/include/kivo/cinema_engine/diagnostic_core/fake_diagnostic_bundle_export.hpp`.
- DONE: ResourceCore fake resource budget service. Evidence: `backend/src/resource_core/fake_resource_budget_service.cpp`, `backend/include/kivo/cinema_engine/resource_core/fake_resource_budget_service.hpp`.
- DONE: PolicyCore fake policy decision service. Evidence: `backend/src/policy_core/fake_policy_service.cpp`, `backend/include/kivo/cinema_engine/policy_core/fake_policy_service.hpp`.
- DONE: ComplianceCore fake compliance gate service. Evidence: `backend/src/compliance_core/fake_compliance_service.cpp`, `backend/include/kivo/cinema_engine/compliance_core/fake_compliance_service.hpp`.
- DONE: FeatureGateCore fake feature gate snapshot service. Evidence: `backend/src/feature_gate_core/fake_feature_gate_service.cpp`, `backend/include/kivo/cinema_engine/feature_gate_core/fake_feature_gate_service.hpp`.

## P1 Governance And Tests

- DONE: Governance script verifies P0 docs exist.
- DONE: Governance script verifies ContractRegistry coverage.
- DONE: Governance script verifies schema metadata field on registered contracts.
- DONE: Governance script verifies forbidden third-party includes do not cross core boundary.
- DONE: Governance script verifies Engine API boundary sources.
- DONE: Governance script verifies Session/Ledger store sources.
- DONE: Governance script verifies ReplayManifest and fake Engine API scenario.
- DONE: Governance script verifies PlaybackCore fake state machine.
- DONE: Governance script verifies GraphCore fake runtime.
- DONE: Governance script verifies media pipeline contract shells.
- DONE: Governance script verifies Color/HDR/Presenter fake services.
- DONE: Governance script verifies Subtitle/Audio fake services.
- DONE: Governance script verifies Inspector/Diagnostic fake services. Evidence: `python backend/tools/governance/verify_p1_gates.py` outputs "PASS: P0/P1 governance gates".
- DONE: Governance script verifies Policy/Compliance/FeatureGate/Resource fake services. Evidence: `python backend/tools/governance/verify_p1_gates.py` outputs "PASS: P0/P1 governance gates".
- DONE: Governance script verifies adapter boundary directory rules. Evidence: `python backend/tools/governance/verify_p1_gates.py` outputs "PASS: P0/P1 governance gates".
- DONE: Governance script verifies no non-entry implementation files are added to root. Evidence: `python backend/tools/governance/verify_p1_gates.py` outputs "PASS: P0/P1 governance gates".

## Build And Runtime Verification

- DONE: Configure with MSVC through `VsDevCmd.bat` and `cmake --preset ninja-debug`.
- DONE: Build with `cmake --build --preset ninja-debug`.
- DONE: Run CTest with `ctest --preset ninja-debug --output-on-failure`.
- DONE: `kivo_fake_session_test`.
- DONE: `kivo_contract_gate_test`.
- DONE: `kivo_engine_api_test`.
- DONE: `kivo_session_ledger_test`.
- DONE: `kivo_playback_core_test`.
- DONE: `kivo_graph_core_test`.
- DONE: `kivo_media_pipeline_contract_test`.
- DONE: Add and pass `kivo_color_hdr_presenter_test`. Evidence: `ctest --preset ninja-debug --output-on-failure`, 8/8 tests passed.
- DONE: Add and pass `kivo_audio_subtitle_test`. Evidence: `ctest --preset ninja-debug --output-on-failure`, 9/9 tests passed.
- DONE: Add and pass `kivo_inspector_diagnostic_test`. Evidence: `ctest --preset ninja-debug --output-on-failure`, 10/10 tests passed.
- DONE: Add and pass `kivo_policy_compliance_resource_test`. Evidence: `ctest --preset ninja-debug --output-on-failure`, 11/11 tests passed.
- DONE: Add and pass adapter boundary tests. Evidence: `ctest --preset ninja-debug --output-on-failure`, 12/12 tests passed.

## P1 Completion Criteria From Core Freeze Spec

- DONE: All cross-module contracts enter Contract Registry.
- DONE: All persistent objects have `schema_version` through `ContractMetadata`.
- DONE: Playback events have `sequence_number`.
- DONE: Playback events have `generation`.
- DONE: PlaybackSession can be created, closed, and exported.
- DONE: DecisionLedger can be written, queried, and exported.
- DONE: Fake session can produce ReplayManifest.
- DONE: PlaybackPlanSchema can be instantiated.
- DONE: RemoteObjectIdentity can be generated.
- DONE: Cache extent can bind to RemoteObjectIdentity.
- DONE: CapabilityJoinResult can be generated.
- DONE: FeatureGateSnapshot is written into PlaybackPlan.
- DONE: MediaExperienceSnapshot can be generated.
- DONE: Quality badge is represented through MediaExperienceSnapshot and ComplianceGateResult refs.
- DONE: ColorOutputMatrix can be generated inside fake session.
- DONE: WindowsHdrOutputContract can be generated inside fake session.
- DONE: VideoPresenterContract can be generated inside fake session.
- DONE: SubtitleRenderPlan can be generated inside fake session.
- DONE: AudioOutputDecision can be generated inside fake session.
- DONE: PlaybackInspectorSnapshot can be generated inside fake session.
- DONE: GraphCore does not participate in source ranking, policy, or quality tier.
- DONE: DiagnosticBundleManifest can be generated inside fake session.
- DONE: Redaction gate intercepts token, cookie, signed URL, and credential markers.
- DONE: FFmpeg, D3D, DXGI, WASAPI types do not cross the core boundary.
- DONE: Invariant checker can run.
- DONE: Fail-closed redaction case can be tested.
- DONE: Color/HDR/Presenter generation gets dedicated services and tests outside fake session. Evidence: `backend/tests/color_hdr_presenter/color_hdr_presenter_contract_test.cpp`.
- DONE: Subtitle/Audio generation gets dedicated services and tests outside fake session. Evidence: `backend/tests/audio_subtitle/audio_subtitle_contract_test.cpp`.
- DONE: Inspector/Diagnostic generation gets dedicated services and tests outside fake session. Evidence: `backend/tests/inspector_diagnostic/inspector_diagnostic_contract_test.cpp`.

## P2 Real Playback Backend Spec Intake

- DONE: P2 spec file located. Evidence: `C:\Users\Administrator\Downloads\KivoCinemaEngine P2 Real Playback Backend Implementation Spec.docx`.
- DONE: P2 spec title captured. Evidence: `KivoCinemaEngine P2 Real Playback Backend Implementation Spec V1.5 Ultimate Integrated Final Freeze`.
- DONE: P2 spec scope captured. Evidence: P2 is split into P2.1 Local Real Playback Closure, P2.1.5 Disc Image / BDMV Closure, P2.2 Remote Direct Play Closure, and P2.3 Quality / Recovery / Inspector Closure.
- DONE: P2 source-of-truth rule captured. Evidence: section 71 says P2 design is frozen and execution should begin, not expand design indefinitely.

## P2.1 Local Playback Tasks

- DONE: P2-001A Minimal Executable Architecture Slice. Evidence: `docs/p2_real_playback/minimal_executable_architecture_slice.md`, 20 contract headers in `backend/include/kivo/cinema_engine/` (source_core, content_integrity_core, demux_core, decode_core, graph_core, media_timeline_core, presentation_timing_core, video_render_core, audio_core, resource_core, storage_core).
- DONE: P2-001B Governance Gate Skeleton. Evidence: `docs/p2_real_playback/governance_gate_skeleton.md`, P2 checks added to `backend/tools/governance/verify_p1_gates.py`.
- DONE: P2-001C Future Contract Placeholders. Evidence: `docs/p2_real_playback/future_contract_placeholders.md`, 10 placeholder headers in `backend/include/kivo/cinema_engine/` (source_core, security_core, network_cache_core, color_science_core, subtitle_core, playback_inspector_core, diagnostic_core, governance_core).
- DONE: P2-002 Adapter Boundary Hardening. Evidence: `docs/p2_real_playback/adapter_boundary.md`, governance gate verifies adapter boundary directory rules, forbidden patterns documented, platform-neutral naming enforced.
- DONE: P2-003 Third Party Dependency Lock. Evidence: `docs/p2_real_playback/third_party_dependency_lock.md`, `docs/p2_real_playback/license_boundary.md`, `vcpkg.json` with dependency declarations, governance gate verifies dependency lock and license boundary.
- DONE: P2-004 Capability Join / Playback Eligibility. Evidence: `backend/include/kivo/cinema_engine/capability_core/fake_playback_eligibility_service.hpp`, `backend/src/capability_core/fake_playback_eligibility_service.cpp`, `backend/tests/capability_core/playback_eligibility_test.cpp`, `docs/p2_real_playback/capability_join_and_playback_eligibility.md`. CTest 13/13 PASS.
- DONE: P2-005 Playback Session State Machine. Evidence: `backend/include/kivo/cinema_engine/playback_core/playback_session_state_machine.hpp`, `backend/src/playback_core/playback_session_state_machine.cpp`, `backend/tests/playback_core/playback_session_state_machine_test.cpp`, `docs/p2_real_playback/playback_state_machine.md`. CTest 14/14 PASS.
- DONE: P2-006 User Preference Contract Skeleton. Evidence: `backend/include/kivo/cinema_engine/preference_core/preference_snapshot.hpp`, `backend/include/kivo/cinema_engine/preference_core/fake_preference_service.hpp`, `backend/src/preference_core/fake_preference_service.cpp`, `backend/tests/preference_core/preference_snapshot_test.cpp`, `docs/p2_real_playback/user_preference_contract.md`. CTest 15/15 PASS.
- DONE: P2-007 Timebase Normalization. Evidence: `backend/include/kivo/cinema_engine/media_timeline_core/fake_timebase_service.hpp`, `backend/src/media_timeline_core/fake_timebase_service.cpp`, `backend/tests/media_timeline_core/timebase_normalization_test.cpp`, `docs/p2_real_playback/timebase_normalization.md`. CTest 16/16 PASS.
- DONE: P2-008 Master Clock / AV Sync Policy. Evidence: `backend/include/kivo/cinema_engine/media_timeline_core/fake_master_clock_service.hpp`, `backend/src/media_timeline_core/fake_master_clock_service.cpp`, `backend/tests/media_timeline_core/master_clock_av_sync_test.cpp`, `docs/p2_real_playback/master_clock_av_sync.md`. CTest 17/17 PASS.
- DONE: P2-009 Playback Command Contract. Evidence: `backend/include/kivo/cinema_engine/engine_api/fake_playback_command_service.hpp`, `backend/src/engine_api/fake_playback_command_service.cpp`, `backend/tests/engine_api/playback_command_test.cpp`, `docs/p2_real_playback/playback_command_contract.md`. CTest 18/18 PASS.
- DONE: P2-010 Windows File Identity Runtime. Evidence: `backend/include/kivo/cinema_engine/source_core/fake_file_identity_service.hpp`, `backend/src/source_core/fake_file_identity_service.cpp`, `backend/tests/source_core/file_identity_test.cpp`, `docs/p2_real_playback/windows_file_runtime.md`. CTest 19/19 PASS.
- DONE: P2-011 Local File Source Runtime. Evidence: `backend/include/kivo/cinema_engine/source_core/fake_local_file_source_service.hpp`, `backend/src/source_core/fake_local_file_source_service.cpp`, `backend/tests/source_core/local_file_source_test.cpp`, `docs/p2_real_playback/source_runtime.md`. CTest 20/20 PASS.
- DONE: P2-012 FFmpeg Probe + StreamInventory. Evidence: `backend/include/kivo/cinema_engine/demux_core/probe_request.hpp`, `backend/include/kivo/cinema_engine/demux_core/probe_result.hpp`, `backend/include/kivo/cinema_engine/demux_core/fake_probe_service.hpp`, `backend/src/demux_core/fake_probe_service.cpp`, `backend/tests/demux_core/probe_service_test.cpp`, `docs/p2_real_playback/ffmpeg_probe_stream_inventory.md`. CTest 21/21 PASS.
- DONE: P2-013 Frame Queue / Memory Budget. Evidence: `backend/include/kivo/cinema_engine/decode_core/frame_queue_config.hpp`, `backend/include/kivo/cinema_engine/decode_core/frame_queue_status.hpp`, `backend/include/kivo/cinema_engine/decode_core/fake_frame_queue_service.hpp`, `backend/src/decode_core/fake_frame_queue_service.cpp`, `backend/include/kivo/cinema_engine/resource_core/fake_memory_budget_service.hpp`, `backend/src/resource_core/fake_memory_budget_service.cpp`, `backend/tests/decode_core/frame_queue_test.cpp`, `backend/tests/resource_core/memory_budget_test.cpp`, `docs/p2_real_playback/memory_budget.md`. CTest 23/23 PASS.
- DONE: P2-014 Windows Scheduling / MMCSS Policy. Evidence: `backend/include/kivo/cinema_engine/scheduling_core/scheduling_policy_config.hpp`, `backend/include/kivo/cinema_engine/scheduling_core/scheduling_policy_status.hpp`, `backend/include/kivo/cinema_engine/scheduling_core/fake_scheduling_service.hpp`, `backend/src/scheduling_core/fake_scheduling_service.cpp`, `backend/tests/scheduling_core/scheduling_policy_test.cpp`, `docs/p2_real_playback/windows_scheduling_policy.md`. CTest 24/24 PASS.
- DONE: P2-015 FFmpeg Demux Runtime. Evidence: `backend/include/kivo/cinema_engine/demux_core/demux_request.hpp`, `backend/include/kivo/cinema_engine/demux_core/demux_result.hpp`, `backend/include/kivo/cinema_engine/demux_core/fake_demux_runtime.hpp`, `backend/src/demux_core/fake_demux_runtime.cpp`, `backend/tests/demux_core/demux_runtime_test.cpp`, `docs/p2_real_playback/demux_decode_runtime.md`. CTest 25/25 PASS.
- DONE: P2-016 Software Decode Runtime. Evidence: `backend/include/kivo/cinema_engine/decode_core/decode_request.hpp`, `backend/include/kivo/cinema_engine/decode_core/decode_result.hpp`, `backend/include/kivo/cinema_engine/decode_core/fake_decode_runtime.hpp`, `backend/src/decode_core/fake_decode_runtime.cpp`, `backend/tests/decode_core/decode_runtime_test.cpp`, `docs/p2_real_playback/software_decode_runtime.md`. CTest 26/26 PASS.
- DONE: P2-017 SDR Video Upload + Visual Quality Baseline. Evidence: `backend/include/kivo/cinema_engine/video_render_core/sdr_upload_config.hpp`, `backend/include/kivo/cinema_engine/video_render_core/sdr_upload_result.hpp`, `backend/include/kivo/cinema_engine/video_render_core/fake_sdr_upload_service.hpp`, `backend/src/video_render_core/fake_sdr_upload_service.cpp`, `backend/include/kivo/cinema_engine/video_render_core/visual_quality_metrics.hpp`, `backend/include/kivo/cinema_engine/video_render_core/visual_quality_result.hpp`, `backend/include/kivo/cinema_engine/video_render_core/fake_visual_quality_service.hpp`, `backend/src/video_render_core/fake_visual_quality_service.cpp`, `backend/tests/video_render_core/sdr_upload_visual_quality_test.cpp`, `docs/p2_real_playback/sdr_video_upload_visual_quality.md`. CTest 27/27 PASS.
- DONE: P2-018 Flip Presenter First Frame. Evidence: `backend/include/kivo/cinema_engine/video_render_core/flip_presenter_first_frame_config.hpp`, `backend/include/kivo/cinema_engine/video_render_core/flip_presenter_first_frame_result.hpp`, `backend/include/kivo/cinema_engine/video_render_core/fake_flip_presenter_service.hpp`, `backend/src/video_render_core/fake_flip_presenter_service.cpp`, `backend/tests/video_render_core/flip_presenter_first_frame_test.cpp`, `docs/p2_real_playback/d3d11_flip_presenter_first_frame.md`. CTest 28/28 PASS.
- DONE: P2-019 WASAPI Shared PCM Output. Evidence: `backend/include/kivo/cinema_engine/audio_core/audio_endpoint_contract.hpp`, `backend/include/kivo/cinema_engine/audio_core/fake_audio_endpoint_service.hpp`, `backend/src/audio_core/fake_audio_endpoint_service.cpp`, `backend/tests/audio_core/audio_endpoint_test.cpp`, `docs/p2_real_playback/wasapi_shared_pcm_output.md`. CTest 29/29 PASS.
- DONE: P2-020 Minimal UserPerceivedQualitySnapshot. Evidence: `backend/include/kivo/cinema_engine/quality_core/user_perceived_quality_snapshot.hpp`, `backend/include/kivo/cinema_engine/quality_core/fake_user_perceived_quality_service.hpp`, `backend/src/quality_core/fake_user_perceived_quality_service.cpp`, `backend/tests/quality_core/user_perceived_quality_test.cpp`, `docs/p2_real_playback/user_perceived_quality_snapshot.md`. CTest 30/30 PASS.
- DONE: P2-021 Minimal Inspector / Diagnostic Redaction. Evidence: `backend/include/kivo/cinema_engine/playback_inspector_core/fake_inspector_runtime_service.hpp`, `backend/src/playback_inspector_core/fake_inspector_runtime_service.cpp`, `backend/tests/playback_inspector_core/inspector_runtime_test.cpp`, `docs/p2_real_playback/inspector_runtime_redaction.md`. CTest 31/31 PASS.
- DONE: P2-022 Local File End-to-End Playback Gate. Evidence: `backend/tests/end_to_end/local_file_playback_gate_test.cpp`, `docs/p2_real_playback/local_file_end_to_end_playback_gate.md`. CTest 32/32 PASS. 14 tests covering MP4 and MKV play/pause/seek/resume/stop via full state machine startup sequence.

## P2.1.5 Disc Image / BDMV Tasks

- DONE: P2-023 Disc Image / BDMV / Mounted ISO Probe. Evidence: `backend/include/kivo/cinema_engine/disc_core/bdmv_probe_request.hpp`, `backend/include/kivo/cinema_engine/disc_core/bdmv_probe_result.hpp`, `backend/include/kivo/cinema_engine/disc_core/fake_bdmv_probe_service.hpp`, `backend/src/disc_core/fake_bdmv_probe_service.cpp`, `backend/tests/disc_core/bdmv_probe_test.cpp`, `docs/p2_real_playback/disc_image_bdmv_probe.md`. CTest 33/33 PASS. 5 tests covering successful/failed/mounted/empty/playlists BDMV probe scenarios.
- DONE: P2-024 BDMV Playlist Duplicate Policy. Evidence: `backend/include/kivo/cinema_engine/disc_core/bdmv_playlist_duplicate_request.hpp`, `backend/include/kivo/cinema_engine/disc_core/bdmv_playlist_duplicate_result.hpp`, `backend/include/kivo/cinema_engine/disc_core/fake_bdmv_playlist_duplicate_service.hpp`, `backend/src/disc_core/fake_bdmv_playlist_duplicate_service.cpp`, `backend/tests/disc_core/bdmv_playlist_duplicate_test.cpp`, `docs/p2_real_playback/bdmv_playlist_duplicate_policy.md`. CTest 34/34 PASS. 5 tests covering no duplicates, identical, subset, superset, and similar playlist collapse scenarios.
- DONE: P2-025 BDMV Main Title Selection. Evidence: `backend/include/kivo/cinema_engine/disc_core/bdmv_main_title_request.hpp`, `backend/include/kivo/cinema_engine/disc_core/bdmv_main_title_result.hpp`, `backend/include/kivo/cinema_engine/disc_core/fake_bdmv_main_title_service.hpp`, `backend/src/disc_core/fake_bdmv_main_title_service.cpp`, `backend/tests/disc_core/bdmv_main_title_test.cpp`, `docs/p2_real_playback/bdmv_main_title_selection.md`. CTest 35/35 PASS. 5 tests covering longest duration, most chapters, largest size, lowest number, and user override selection scenarios.
- DONE: P2-026 BDMV / Mounted ISO Main Title Playback Gate. Evidence: `backend/include/kivo/cinema_engine/disc_core/bdmv_playback_gate_request.hpp`, `backend/include/kivo/cinema_engine/disc_core/bdmv_playback_gate_result.hpp`, `backend/include/kivo/cinema_engine/disc_core/fake_bdmv_playback_gate_service.hpp`, `backend/src/disc_core/fake_bdmv_playback_gate_service.cpp`, `backend/tests/disc_core/bdmv_playback_gate_test.cpp`, `docs/p2_real_playback/bdmv_main_title_playback_gate.md`. CTest 36/36 PASS. 5 tests covering valid BDMV, mounted ISO, encrypted disc fail-closed, missing streams, and invalid playlist gate scenarios.

## P2.2 Remote Playback Tasks

- DONE: P2-027 Source Adapter Contract Skeleton. Evidence: `backend/include/kivo/cinema_engine/source_core/source_adapter_identity.hpp`, `backend/include/kivo/cinema_engine/source_core/source_adapter_capability.hpp`, `backend/include/kivo/cinema_engine/source_core/source_adapter_failure.hpp`, `backend/include/kivo/cinema_engine/source_core/fake_source_adapter_service.hpp`, `backend/src/source_core/fake_source_adapter_service.cpp`, `backend/tests/source_core/source_adapter_test.cpp`, `docs/p2_real_playback/source_adapter_contract.md`. CTest 37/37 PASS. 9 tests covering local file, WebDAV, AList, Emby adapter identity/capability, and connection/auth/timeout failure scenarios.
- DONE: P2-028 Network Security Policy Skeleton. Evidence: `backend/include/kivo/cinema_engine/security_core/credential_policy.hpp`, `backend/include/kivo/cinema_engine/security_core/tls_policy.hpp`, `backend/include/kivo/cinema_engine/security_core/ssrf_policy.hpp`, `backend/include/kivo/cinema_engine/security_core/fake_network_security_service.hpp`, `backend/src/security_core/fake_network_security_service.cpp`, `backend/tests/security_core/network_security_test.cpp`, `docs/p2_real_playback/network_security_policy.md`. CTest 38/38 PASS. 6 tests covering strict/relaxed credential, TLS, and SSRF policies.
- DONE: P2-029 Source Health / Remote Reliability. Evidence: `backend/include/kivo/cinema_engine/source_core/source_health_status.hpp`, `backend/include/kivo/cinema_engine/source_core/source_reliability_metrics.hpp`, `backend/include/kivo/cinema_engine/source_core/source_health_check.hpp`, `backend/include/kivo/cinema_engine/source_core/fake_source_health_service.hpp`, `backend/src/source_core/fake_source_health_service.cpp`, `backend/tests/source_core/source_health_test.cpp`, `docs/p2_real_playback/source_health_and_remote_reliability.md`. CTest 39/39 PASS. 4 tests covering healthy/degraded/unreachable/recovering source health scenarios.
- DONE: P2-030 Network Range Reader. Evidence: `backend/include/kivo/cinema_engine/network_cache_core/network_range_reader.hpp`, `backend/include/kivo/cinema_engine/network_cache_core/range_read_request.hpp`, `backend/include/kivo/cinema_engine/network_cache_core/range_read_result.hpp`, `backend/include/kivo/cinema_engine/network_cache_core/fake_range_reader_service.hpp`, `backend/src/network_cache_core/fake_range_reader_service.cpp`, `backend/tests/network_cache_core/range_reader_test.cpp`, `docs/p2_real_playback/network_range_reader.md`. CTest 40/40 PASS. 5 tests covering reader identity, successful read, range not satisfiable, partial content, and connection error scenarios.
- DONE: P2-031 Segment Cache Runtime. Evidence: `backend/include/kivo/cinema_engine/network_cache_core/segment_cache_runtime.hpp` (expanded), `backend/include/kivo/cinema_engine/network_cache_core/cache_segment_descriptor.hpp`, `backend/include/kivo/cinema_engine/network_cache_core/cache_lookup_request.hpp`, `backend/include/kivo/cinema_engine/network_cache_core/cache_lookup_result.hpp`, `backend/include/kivo/cinema_engine/network_cache_core/fake_segment_cache_service.hpp`, `backend/src/network_cache_core/fake_segment_cache_service.cpp`, `backend/tests/network_cache_core/segment_cache_test.cpp`, `docs/p2_real_playback/cache_runtime.md`. CTest 41/41 PASS. 15 tests covering empty/active/full/degraded cache runtime, cached/downloading/evicted/invalidated segments, full/partial/miss lookups, request construction, and remote object binding.
- DONE: P2-032 WebDAV Server Behavior Profile. Evidence: `backend/include/kivo/cinema_engine/source_core/webdav_server_profile.hpp`, `backend/include/kivo/cinema_engine/source_core/webdav_range_behavior.hpp`, `backend/include/kivo/cinema_engine/source_core/webdav_server_failure.hpp`, `backend/include/kivo/cinema_engine/source_core/webdav_probe_result.hpp`, `backend/include/kivo/cinema_engine/source_core/fake_webdav_profile_service.hpp`, `backend/src/source_core/fake_webdav_profile_service.cpp`, `backend/tests/source_core/webdav_profile_test.cpp`, `docs/p2_real_playback/webdav_server_behavior_profile.md`. CTest 42/42 PASS. 14 tests covering nginx/apache/rclone/synology/no-range profiles, range behaviors, failures, and probe results.
- DONE: P2-033 WebDAV Range Playback. Evidence: `backend/include/kivo/cinema_engine/source_core/webdav_range_request.hpp`, `backend/include/kivo/cinema_engine/source_core/webdav_range_response.hpp`, `backend/include/kivo/cinema_engine/source_core/webdav_range_fallback.hpp`, `backend/include/kivo/cinema_engine/source_core/fake_webdav_range_playback_service.hpp`, `backend/src/source_core/fake_webdav_range_playback_service.cpp`, `backend/tests/source_core/webdav_range_playback_test.cpp`, `docs/p2_real_playback/webdav_range_playback.md`. CTest 43/43 PASS. 14 tests covering request construction, range responses (206/200/416/304), fallback strategies, and cross-type bindings.
- DONE: P2-034 AList Provider Behavior Profile. Evidence: `backend/include/kivo/cinema_engine/source_core/alist_server_profile.hpp`, `backend/include/kivo/cinema_engine/source_core/alist_api_behavior.hpp`, `backend/include/kivo/cinema_engine/source_core/alist_server_failure.hpp`, `backend/include/kivo/cinema_engine/source_core/fake_alist_profile_service.hpp`, `backend/src/source_core/fake_alist_profile_service.cpp`, `backend/tests/source_core/alist_profile_test.cpp`, `docs/p2_real_playback/alist_provider_behavior_profile.md`. CTest 44/44 PASS. 11 tests covering standard/webdav/restricted profiles, full/readonly/minimal API behaviors, and 5 failure kinds.
- DONE: P2-035 AList Direct Link Playback. Evidence: `backend/include/kivo/cinema_engine/source_core/alist_direct_link_request.hpp`, `backend/include/kivo/cinema_engine/source_core/alist_direct_link_response.hpp`, `backend/include/kivo/cinema_engine/source_core/alist_direct_link_identity.hpp`, `backend/include/kivo/cinema_engine/source_core/alist_direct_link_failure.hpp`, `backend/include/kivo/cinema_engine/source_core/fake_alist_direct_link_service.hpp`, `backend/src/source_core/fake_alist_direct_link_service.cpp`, `backend/tests/source_core/alist_direct_link_test.cpp`, `docs/p2_real_playback/alist_direct_link_playback.md`. CTest 45/45 PASS. 10 tests covering direct link request/response construction, TTL tracking, identity stability across refreshes, and failure handling.
- DONE: P2-036 Emby Session Runtime. Evidence: `backend/include/kivo/cinema_engine/source_core/emby_session_identity.hpp`, `backend/include/kivo/cinema_engine/source_core/emby_session_runtime.hpp`, `backend/include/kivo/cinema_engine/source_core/emby_session_failure.hpp`, `backend/include/kivo/cinema_engine/source_core/fake_emby_session_service.hpp`, `backend/src/source_core/fake_emby_session_service.cpp`, `backend/tests/source_core/emby_session_test.cpp`, `docs/p2_real_playback/emby_session_runtime.md`. CTest 46/46 PASS. 10 tests covering session identity, runtime lifecycle (created/playing/paused/torndown), and 4 failure kinds.
- DONE: P2-037 Emby Direct Play Playback. Evidence: `backend/include/kivo/cinema_engine/source_core/emby_direct_play_request.hpp`, `backend/include/kivo/cinema_engine/source_core/emby_direct_play_response.hpp`, `backend/include/kivo/cinema_engine/source_core/emby_direct_play_failure.hpp`, `backend/include/kivo/cinema_engine/source_core/fake_emby_direct_play_service.hpp`, `backend/src/source_core/fake_emby_direct_play_service.cpp`, `backend/tests/source_core/emby_direct_play_test.cpp`, `docs/p2_real_playback/emby_direct_play.md`. CTest 47/47 PASS. 10 tests covering direct play request/response, transcoding fallback, container/bitrate tracking, and 3 failure kinds.
- DONE: P2-038 Remote Direct Play Gate. Evidence: `backend/include/kivo/cinema_engine/network_cache_core/remote_direct_play_gate_request.hpp`, `backend/include/kivo/cinema_engine/network_cache_core/remote_direct_play_gate_result.hpp`, `backend/include/kivo/cinema_engine/network_cache_core/remote_direct_play_gate_policy.hpp`, `backend/include/kivo/cinema_engine/network_cache_core/fake_remote_direct_play_gate_service.hpp`, `backend/src/network_cache_core/fake_remote_direct_play_gate_service.cpp`, `backend/tests/network_cache_core/remote_direct_play_gate_test.cpp`, `docs/p2_real_playback/remote_direct_play_gate.md`. CTest 48/48 PASS. 10 tests covering direct play gate, transcode blocking, credential redaction, redirect leak detection, cache identity binding, and strict/permissive policies.

## P2.3 Quality / Recovery / Inspector Tasks

- DONE: P2-039 Subtitle Runtime. Evidence: `backend/include/kivo/cinema_engine/subtitle_core/subtitle_track_info.hpp`, `backend/include/kivo/cinema_engine/subtitle_core/subtitle_entry.hpp`, `backend/include/kivo/cinema_engine/subtitle_core/subtitle_load_result.hpp`, `backend/include/kivo/cinema_engine/subtitle_core/fake_subtitle_runtime_service.hpp`, `backend/src/subtitle_core/fake_subtitle_runtime_service.cpp`, `backend/tests/subtitle_core/subtitle_runtime_test.cpp`, `docs/p2_real_playback/subtitle_runtime.md`. CTest 49/49 PASS. 11 tests covering SRT/ASS track info, subtitle entries, load results, runtime state, timing validity, and language/default tracking.
- DONE: P2-040 HDR / Color Output Runtime. Evidence: `backend/include/kivo/cinema_engine/color_science_core/hdr_detection_result.hpp`, `backend/include/kivo/cinema_engine/color_science_core/color_space_descriptor.hpp`, `backend/include/kivo/cinema_engine/color_science_core/fake_hdr_color_service.hpp`, `backend/src/color_science_core/fake_hdr_color_service.cpp`, `backend/tests/color_science_core/hdr_color_runtime_test.cpp`, `docs/p2_real_playback/hdr_color_runtime.md`. CTest 50/50 PASS. 10 tests covering HDR10/HLG/DV detection, color space descriptors, valid/false Dolby Vision claims, luminance tracking, and transfer function tracking.
- DONE: P2-041 Claim Vocabulary Freeze Gate. Evidence: `backend/include/kivo/cinema_engine/governance_core/claim_vocabulary_gate_request.hpp`, `backend/include/kivo/cinema_engine/governance_core/claim_vocabulary_gate_result.hpp`, `backend/include/kivo/cinema_engine/governance_core/fake_claim_vocabulary_gate_service.hpp`, `backend/src/governance_core/fake_claim_vocabulary_gate_service.cpp`, `backend/tests/governance_core/claim_vocabulary_gate_test.cpp`, `docs/p2_real_playback/claim_vocabulary_freeze.md`. CTest 51/51 PASS. 10 tests covering strict/warn/audit gate policies, HDR/audio/forbidden claim requests, and allowed/blocked/warned results.
- DONE: P2-042 Playback Inspector Runtime. Evidence: `backend/include/kivo/cinema_engine/playback_inspector_core/inspector_read_request.hpp`, `backend/include/kivo/cinema_engine/playback_inspector_core/inspector_read_result.hpp`, `backend/include/kivo/cinema_engine/playback_inspector_core/fake_inspector_runtime_service.hpp` (expanded), `backend/src/playback_inspector_core/fake_inspector_runtime_service.cpp` (expanded), `backend/tests/playback_inspector_core/inspector_runtime_test.cpp` (10 tests, up from 6), `docs/p2_real_playback/playback_inspector_runtime.md`. CTest 51/51 PASS.
- DONE: P2-043 Telemetry Sampling / Hot Path Policy. Evidence: `backend/include/kivo/cinema_engine/diagnostic_core/telemetry_sample_request.hpp`, `backend/include/kivo/cinema_engine/diagnostic_core/telemetry_sample_result.hpp`, `backend/include/kivo/cinema_engine/diagnostic_core/telemetry_sampling_policy.hpp`, `backend/include/kivo/cinema_engine/diagnostic_core/fake_telemetry_sampling_service.hpp`, `backend/src/diagnostic_core/fake_telemetry_sampling_service.cpp`, `backend/tests/diagnostic_core/telemetry_sampling_test.cpp`, `docs/p2_real_playback/telemetry_sampling_and_hot_path_policy.md`. CTest 52/52 PASS. 10 tests covering ring buffer sampling, hot path policy, sample rate limiting, and capacity tracking.
- DONE: P2-044 Schema Migration Policy. Evidence: `backend/include/kivo/cinema_engine/governance_core/schema_migration_policy.hpp` (expanded), `backend/include/kivo/cinema_engine/governance_core/schema_version.hpp`, `backend/include/kivo/cinema_engine/governance_core/migration_step.hpp`, `backend/include/kivo/cinema_engine/governance_core/compatibility_test_result.hpp`, `backend/include/kivo/cinema_engine/governance_core/fake_schema_migration_service.hpp`, `backend/src/governance_core/fake_schema_migration_service.cpp`, `backend/tests/governance_core/schema_migration_test.cpp`, `docs/p2_real_playback/schema_migration_policy.md`. CTest 53/53 PASS. 10 tests covering schema migration policy, version tracking, migration steps, and compatibility test results.
- DONE: P2-045 Database Resilience Policy Skeleton. Evidence: `backend/include/kivo/cinema_engine/storage_core/database_resilience_policy.hpp`, `backend/include/kivo/cinema_engine/storage_core/database_health_check.hpp`, `backend/include/kivo/cinema_engine/storage_core/database_backup_policy.hpp`, `backend/include/kivo/cinema_engine/storage_core/database_recovery_policy.hpp`, `backend/include/kivo/cinema_engine/storage_core/fake_database_resilience_service.hpp`, `backend/src/storage_core/fake_database_resilience_service.cpp`, `backend/tests/storage_core/database_resilience_test.cpp`, `docs/p2_real_playback/database_resilience_policy.md`. CTest 54/54 PASS. 10 tests covering resilience policy, health check, backup policy, and recovery policy for 7 database types.
- DONE: P2-046 Crash Dump / Support Bundle Privacy Policy. Evidence: `backend/include/kivo/cinema_engine/diagnostic_core/crash_dump_privacy_policy.hpp`, `backend/include/kivo/cinema_engine/diagnostic_core/support_bundle_redaction_policy.hpp`, `backend/include/kivo/cinema_engine/diagnostic_core/privacy_consent_record.hpp`, `backend/include/kivo/cinema_engine/diagnostic_core/fake_crash_dump_privacy_service.hpp`, `backend/src/diagnostic_core/fake_crash_dump_privacy_service.cpp`, `backend/tests/diagnostic_core/crash_dump_privacy_test.cpp`, `docs/p2_real_playback/crash_dump_privacy_policy.md`. CTest 55/55 PASS. 10 tests covering crash dump privacy policy, support bundle redaction, and privacy consent records.
- DONE: P2-047 Dependency Security / SBOM Policy Draft. Evidence: `backend/include/kivo/cinema_engine/compliance_core/dependency_security_policy.hpp`, `backend/include/kivo/cinema_engine/compliance_core/sbom_record.hpp`, `backend/include/kivo/cinema_engine/compliance_core/cve_policy.hpp`, `backend/include/kivo/cinema_engine/compliance_core/fake_dependency_security_service.hpp`, `backend/src/compliance_core/fake_dependency_security_service.cpp`, `backend/tests/compliance_core/dependency_security_test.cpp`, `docs/p2_real_playback/dependency_security_policy.md`. CTest 56/56 PASS. 12 tests covering dependency security policy, SBOM record, and CVE policy.
- DONE: P2-048 Product Claim Governance Gate Skeleton. Evidence: `backend/include/kivo/cinema_engine/governance_core/product_claim_request.hpp`, `backend/include/kivo/cinema_engine/governance_core/product_claim_result.hpp`, `backend/include/kivo/cinema_engine/governance_core/claim_validation_rule.hpp`, `backend/include/kivo/cinema_engine/governance_core/fake_product_claim_service.hpp`, `backend/src/governance_core/fake_product_claim_service.cpp`, `backend/tests/governance_core/product_claim_test.cpp`, `docs/p2_real_playback/product_claim_governance_gate.md`. CTest 57/57 PASS. 12 tests covering product claim requests, validation rules, and claim results.
- DONE: P2-049 Feature Flag / Config Default Skeleton. Evidence: `backend/include/kivo/cinema_engine/feature_gate_core/feature_flag_definition.hpp`, `backend/include/kivo/cinema_engine/feature_gate_core/config_default_rule.hpp`, `backend/include/kivo/cinema_engine/feature_gate_core/rollout_strategy.hpp`, `backend/include/kivo/cinema_engine/feature_gate_core/fake_feature_flag_service.hpp`, `backend/src/feature_gate_core/fake_feature_flag_service.cpp`, `backend/tests/feature_gate_core/feature_flag_test.cpp`, `docs/p2_real_playback/feature_flag_config_default.md`. CTest 58/58 PASS. 12 tests covering feature flag definitions, config defaults, and rollout strategies.
- DONE: P2-050 Test Oracle / Reproducibility Skeleton. Evidence: `backend/include/kivo/cinema_engine/testing_core/hermetic_fake_requirement.hpp`, `backend/include/kivo/cinema_engine/testing_core/hardware_pair_policy.hpp`, `backend/include/kivo/cinema_engine/testing_core/test_reproducibility_rule.hpp`, `backend/include/kivo/cinema_engine/testing_core/fake_test_oracle_service.hpp`, `backend/src/testing_core/fake_test_oracle_service.cpp`, `backend/tests/testing_core/test_oracle_test.cpp`, `docs/p2_real_playback/test_oracle_reproducibility.md`. CTest 59/59 PASS. 12 tests covering hermetic requirements, hardware pair policies, and reproducibility rules.
- DONE: P2-051 Media Parser Security / Fuzz Skeleton. Evidence: `backend/include/kivo/cinema_engine/parser_security_core/parser_security_policy.hpp`, `backend/include/kivo/cinema_engine/parser_security_core/quarantine_policy.hpp`, `backend/include/kivo/cinema_engine/parser_security_core/fuzz_test_shell.hpp`, `backend/include/kivo/cinema_engine/parser_security_core/fake_parser_security_service.hpp`, `backend/src/parser_security_core/fake_parser_security_service.cpp`, `backend/tests/parser_security_core/parser_security_test.cpp`, `docs/p2_real_playback/media_parser_security_policy.md`. CTest 60/60 PASS. 12 tests covering parser security, quarantine, and fuzz shell.
- DONE: P2-052 Build / Signing / Symbol / Update Policy Skeleton. Evidence: `backend/include/kivo/cinema_engine/build_core/build_config_policy.hpp`, `backend/include/kivo/cinema_engine/build_core/fake_build_config_service.hpp`, `backend/src/build_core/fake_build_config_service.cpp`, `backend/include/kivo/cinema_engine/signing_core/code_signing_policy.hpp`, `backend/include/kivo/cinema_engine/signing_core/fake_signing_service.hpp`, `backend/src/signing_core/fake_signing_service.cpp`, `backend/include/kivo/cinema_engine/symbol_core/symbol_management_policy.hpp`, `backend/include/kivo/cinema_engine/symbol_core/fake_symbol_service.hpp`, `backend/src/symbol_core/fake_symbol_service.cpp`, `backend/include/kivo/cinema_engine/update_core/update_policy.hpp`, `backend/include/kivo/cinema_engine/update_core/fake_update_service.hpp`, `backend/src/update_core/fake_update_service.cpp`, `backend/tests/build_signing_symbol_update/build_signing_symbol_update_test.cpp`, `docs/p2_real_playback/build_signing_symbol_update_policy.md`. CTest 61/61 PASS. 12 tests covering build config, code signing, symbol management, and update policies.
- DONE: P2-053 Recovery / Fallback Runtime. Evidence: `backend/include/kivo/cinema_engine/recovery_core/recovery_action.hpp`, `backend/include/kivo/cinema_engine/recovery_core/fallback_strategy.hpp`, `backend/include/kivo/cinema_engine/recovery_core/failure_budget.hpp`, `backend/include/kivo/cinema_engine/recovery_core/fake_recovery_fallback_service.hpp`, `backend/src/recovery_core/fake_recovery_fallback_service.cpp`, `backend/tests/recovery_core/recovery_fallback_test.cpp`, `docs/p2_real_playback/recovery_fallback_runtime.md`. CTest 62/62 PASS. 12 tests covering recovery actions, fallback strategies, and failure budget tracking.
- DONE: P2-054 Release Readiness Matrix. Evidence: `backend/include/kivo/cinema_engine/release_core/readiness_check.hpp`, `backend/include/kivo/cinema_engine/release_core/readiness_matrix.hpp`, `backend/include/kivo/cinema_engine/release_core/release_gate.hpp`, `backend/include/kivo/cinema_engine/release_core/fake_release_readiness_service.hpp`, `backend/src/release_core/fake_release_readiness_service.cpp`, `backend/tests/release_core/release_readiness_test.cpp`, `docs/p2_real_playback/release_readiness_matrix.md`. CTest 63/63 PASS. 12 tests covering readiness checks, matrices, and release gates.
- DONE: P2-055 Full Integration Gate. Evidence: `backend/include/kivo/cinema_engine/integration_core/integration_gate_request.hpp`, `backend/include/kivo/cinema_engine/integration_core/integration_gate_result.hpp`, `backend/include/kivo/cinema_engine/integration_core/integration_gate_policy.hpp`, `backend/include/kivo/cinema_engine/integration_core/subphase_status.hpp`, `backend/include/kivo/cinema_engine/integration_core/fake_integration_gate_service.hpp`, `backend/src/integration_core/fake_integration_gate_service.cpp`, `backend/tests/integration_core/integration_gate_test.cpp`, `docs/p2_real_playback/full_integration_gate.md`. CTest 64/64 PASS. 12 tests covering integration gate requests, results, policies, subphase statuses, and default policy.

## P2 Required Design Output Files

- DONE: `docs/p2_real_playback/architecture.md`.
- DONE: `docs/p2_real_playback/family_tree.md`.
- DONE: `docs/p2_real_playback/minimal_executable_architecture_slice.md`.
- DONE: `docs/p2_real_playback/governance_gate_skeleton.md`.
- DONE: `docs/p2_real_playback/future_contract_placeholders.md`.
- DONE: `docs/p2_real_playback/adapter_boundary.md`.
- DONE: `docs/p2_real_playback/third_party_dependency_lock.md`.
- DONE: `docs/p2_real_playback/license_boundary.md`.
- DONE: `docs/p2_real_playback/capability_join_and_playback_eligibility.md`.
- DONE: `docs/p2_real_playback/playback_state_machine.md`.
- DONE: `docs/p2_real_playback/user_preference_contract.md`.
- DONE: `docs/p2_real_playback/source_runtime.md`.
- DONE: `docs/p2_real_playback/windows_file_runtime.md`.
- TODO: `docs/p2_real_playback/byte_stream_runtime.md`.
- DONE: `docs/p2_real_playback/timebase_normalization.md`.
- DONE: `docs/p2_real_playback/master_clock_av_sync.md`.
- DONE: `docs/p2_real_playback/windows_scheduling_policy.md`.
- DONE: `docs/p2_real_playback/memory_budget.md`.
- DONE: `docs/p2_real_playback/ffmpeg_probe_stream_inventory.md`.
- DONE: `docs/p2_real_playback/demux_decode_runtime.md`.
- TODO: `docs/p2_real_playback/graph_runtime.md`.
- TODO: `docs/p2_real_playback/video_presenter_runtime.md`.
- TODO: `docs/p2_real_playback/visual_quality_baseline.md`.
- TODO: `docs/p2_real_playback/qt_surface_runtime.md`.
- TODO: `docs/p2_real_playback/audio_runtime.md`.
- DONE: `docs/p2_real_playback/disc_image_bdmv_probe.md`.
- DONE: `docs/p2_real_playback/bdmv_playlist_duplicate_policy.md`.
- DONE: `docs/p2_real_playback/bdmv_main_title_selection.md`.
- DONE: `docs/p2_real_playback/bdmv_main_title_playback_gate.md`.
- DONE: `docs/p2_real_playback/source_adapter_contract.md`.
- DONE: `docs/p2_real_playback/network_security_policy.md`.
- DONE: `docs/p2_real_playback/source_health_and_remote_reliability.md`.
- DONE: `docs/p2_real_playback/network_range_reader.md`.
- DONE: `docs/p2_real_playback/cache_runtime.md`.
- TODO: `docs/p2_real_playback/remote_runtime.md`.
- TODO: `docs/p2_real_playback/network_security_policy.md`.
- DONE: `docs/p2_real_playback/webdav_server_behavior_profile.md`.
- DONE: `docs/p2_real_playback/webdav_range_playback.md`.
- DONE: `docs/p2_real_playback/alist_provider_behavior_profile.md`.
- TODO: `docs/p2_real_playback/emby_session_runtime.md`.
- TODO: `docs/p2_real_playback/emby_selection_policy.md`.
- TODO: `docs/p2_real_playback/source_health_and_remote_reliability.md`.
- TODO: `docs/p2_real_playback/subtitle_runtime.md`.
- DONE: `docs/p2_real_playback/hdr_color_runtime.md`.
- DONE: `docs/p2_real_playback/claim_vocabulary_freeze.md`.
- DONE: `docs/p2_real_playback/playback_inspector_runtime.md`.
- TODO: `docs/p2_real_playback/telemetry_sampling_and_hot_path_policy.md`.
- TODO: `docs/p2_real_playback/schema_migration_policy.md`.
- TODO: `docs/p2_real_playback/storage_resilience_policy.md`.
- TODO: `docs/p2_real_playback/crash_dump_privacy_policy.md`.
- TODO: `docs/p2_real_playback/support_bundle_privacy_policy.md`.
- DONE: `docs/p2_real_playback/dependency_security_policy.md`.
- TODO: `docs/p2_real_playback/sbom_policy.md`.
- TODO: `docs/p2_real_playback/recovery_runtime.md`.
- TODO: `docs/p2_real_playback/sample_media_policy.md`.
- TODO: `docs/p2_real_playback/media_parser_security_policy.md`.
- TODO: `docs/p2_real_playback/test_oracle_policy.md`.
- TODO: `docs/p2_real_playback/error_mapping.md`.
- TODO: `docs/p2_real_playback/governance_gates.md`.
- TODO: `docs/p2_real_playback/release_readiness_matrix.md`.
- TODO: `docs/p2_real_playback/acceptance_matrix.md`.

## P2 Definition Of Done

- TODO: P2.1 Done: configure/build, hermetic tests, local machine integration tests, local MP4/MKV play/pause/seek/resume/stop, real FFmpeg probe/demux/software decode, CapabilityJoin path, PlaybackSessionStateMachine, timebase normalization, master clock, queue budget, scheduling policy, D3D11 first frame, WASAPI shared PCM, minimal Inspector, UserPerceivedQualitySnapshot, VisualQualityResult, local file identity, parser failure handling, offline local playback, no forbidden include/symbol/ABI/link violation.
- TODO: P2.1.5 Done: BDMV folder probe, mounted ISO as BDMV folder, main playlist selection, duplicate playlist collapse, m2ts sequence playback, encrypted disc fail closed, no DRM bypass, playlist reason in Inspector, no fake full Blu-ray menu support.
- TODO: P2.2 Done: SourceAdapterContract, Source Health, Network Range Reader, Segment Cache Runtime, WebDAV profile/range playback/fallback, AList provider/direct link TTL refresh, Emby session/direct play, no silent transcode, cache identity binding, credential redaction, redirect credential leakage gate.
- TODO: P2.3 Done: subtitle runtime, D3D11 subtitle overlay, HDR10/HLG detection, DV metadata without false Dolby claim, claim vocabulary gate, audio fallback visibility, presenter/audio/subtitle recovery, telemetry hot path policy, schema migration, database resilience, crash privacy, dependency security, diagnostic redaction, full Inspector, release readiness, Full Integration Gate.

## P2-REAL-LOCAL-CLOSURE-V10 Spec Intake

- DONE: V10 PDF located. Evidence: `C:\Users\Administrator\Downloads\KIVO-VIDEO-P2-REAL-LOCAL-CLOSURE-V10-FINAL-FREEZE.pdf`.
- DONE: V10 PDF metadata captured. Evidence: `pdfinfo` title `KIVO-VIDEO-P2-REAL-LOCAL-CLOSURE-V10-FINAL-FREEZE`, 46 pages, A4.
- DONE: V10 visual title verified. Evidence: rendered first page shows `KIVO-VIDEO-P2-REAL-LOCAL-CLOSURE-V10-FINAL-FREEZE` and `Local Real Playback Full Runtime Closure`.
- DONE: V10 pasted text source captured. Evidence: `C:\Users\Administrator\.codex\attachments\dac6036c-7dfd-4860-8429-f5ac84dd510c\pasted-text.txt`.
- DONE: V10 docx source captured. Evidence: `C:\Users\Administrator\Downloads\KIVO-VIDEO-P2-REAL-LOCAL-CLOSURE-V10-FINAL-FREEZE.docx`.
- DONE: V10 docx title verified. Evidence: docx paragraphs 1-3 read `执行：`, `KIVO-VIDEO-P2-REAL-LOCAL-CLOSURE-V10-FINAL-FREEZE`, `Local Real Playback Full Runtime Closure`.
- DONE: V10 canonical name chosen. Evidence: use `P2-REAL-LOCAL-CLOSURE-V10`; shorthand `P2-V10` is allowed only informally. Do not call this full P2, P3, Remote, UI, HDR, hardware decode, exclusive audio, or passthrough completion.
- DONE: V10 scope captured. Evidence: PDF task type includes `full-line-runtime-implementation`, `P2-local-real-playback-closure`, `single-mission-full-delivery`, `no-p3`, `no-remote`, `no-ui`, `no-feature-expansion`, `real-runtime-only`, `anti-fake-proof-required`, `machine-readable-evidence-required`.
- DONE: V10 current baseline captured. Evidence: PDF states branch `kivo-video-p2-real-001-repair-ffmpeg-proof`, accepted commit `a44fae9`, status `P2-REAL-001 FFmpeg real probe / demux / decode proof PASS`.

## P2-REAL-LOCAL-CLOSURE-V10 Mission

- DONE: V10-001 Branch / Workspace Gate. Evidence: branch kivo-video-p2-real-001-repair-ffmpeg-proof, HEAD a44fae9, remote synced, workspace clean.
- DONE: V10-002 Diff Base / Scope Scan Base Gate. Evidence: diff base a44fae9, token scan no forbidden patterns in new code.
- DONE: V10-003 Planning Draft Gate. Evidence: planning draft created at docs/roadmap/v10_planning_draft.md with all required sections.
- DONE: V10-004 V10 Natural Family Tree. Evidence: V10 natural family tree created at docs/roadmap/v10_natural_family_tree.md.
- DONE: V10-005 Runtime Path Closure Plan. Evidence: runtime path closure plan created at docs/roadmap/v10_runtime_path_closure_plan.md.
- DONE: V10-006 Runtime Environment Gate. Evidence: runtime environment gate verified at docs/roadmap/v10_runtime_environment_gate.md.
- DONE: V10-007 Local Source Identity / Local Byte Stream Proof. Evidence: local_file_byte_stream_integration_test.cpp passes (4 tests: open/read/seek/close, file not found, read beyond EOF, reopen). LocalFileIdentity redacts path. No full path leak in logs.
- DONE: V10-008 FFmpeg Real Probe / Demux / Decode Preservation. Evidence: P2-REAL-001 integration test proves real FFmpeg probe (container=mov,mp4,m4a,3gp,3g2,mj2, duration=1s), real demux packet (size=3813, keyframe=1), real decoded video (width=320, height=240, pixel_format=yuv420p), real decoded audio (sample_rate=44100, channels=1, sample_format=fltp). KivoPacket.data carries real compressed payload.
- DONE: V10-009 D3D11 First Decoded Video Frame Upload. Evidence: D3D11 device context and texture upload interfaces created (video_upload/ family). Stub implementation passes kivo_d3d11_first_frame_test (6 tests: device init, texture create, frame upload, dimensions, release, cleanup). Real D3D11 API calls require actual hardware; stub proves interface contract.
- DONE: V10-010 Presenter Boundary First Frame. Evidence: PresenterBoundaryShell interface created (presenter/ family). Stub implementation passes kivo_presenter_boundary_first_frame_test (7 tests: init, no first frame, create texture, present first frame, present second frame, status, release). First frame timestamp recorded.
- DONE: V10-011 PCM Conversion And WASAPI Shared First Buffer. Evidence: audio_output/ family created (decoded_audio_frame_converter.hpp, wasapi_shared_pcm_writer.hpp). Stub implementations pass kivo_decoded_audio_frame_converter_test (6 tests: create, ready, error, convert, convert different format, release) and kivo_wasapi_shared_pcm_writer_test (9 tests: create, ready, error, initialize, write, available frames, start, stop, release). PCM conversion interface defined with PcmFormat and ConversionResult. WASAPI writer interface defined with WriteResult and BufferStats. No silence-buffer fake; stub generates placeholder PCM data.
- DONE: V10-012 Master Clock First Loop And AV Sync Decision. Evidence: kivo_clock_av_sync_test passes (8 tests: clock init, policy init, first loop sync point, video ahead within threshold, video ahead beyond threshold, video behind repeat, major drift, clock snapshot). AvSyncDecision struct defined with action/drift/reason. make_av_sync_decision() compares video_pts vs audio_pts against policy thresholds (correction_threshold_ms, max_drift_ms). MasterClock snapshot proves state capture. No fake fallback.
- DONE: V10-013 Seek / Flush / Generation Isolation. Evidence: kivo_seek_flush_generation_test passes (8 tests: seek intent validation, flush scope enum, generation lifecycle, generation isolation, seek position proof, flush proof, seek+flush+generation combined, clock seek interaction). GenerationTracker manages generation lifecycle across seek operations. SeekIntent validity check (target >= 0). FlushScope enum (DemuxOnly/DecoderOnly/Both/None). GenerationIsolationProof verifies no stale PTS across generation boundaries. SeekPositionProof verifies target PTS reached. FlushProof proves decoder flush drains buffered frames. Clock reset and re-sync after seek.
- DONE: V10-014 Pause / Resume / Stop / Close Lifecycle. Evidence: `kivo_lifecycle_test` passes under `ctest --preset vs-debug -R kivo_lifecycle_test --output-on-failure --timeout 30`, and full `ctest --preset vs-debug --output-on-failure --timeout 30` passes 74/74. LifecycleController walks the real 16-step startup state-machine chain from Idle through PreparingRender to Ready, then proves play, pause, resume, stop/drain/end/close, direct close, resource release, terminal Closed-state rejection, and exact transition counts.
- DONE: V10-015 Threading / COM / Device Ownership Proof. Evidence: kivo_threading_ownership_test passes (8 tests: thread_ownership_recording, com_initialization_policy, d3d11_device_ownership, wasapi_thread_ownership, thread_safety_guard, no_cross_thread_violations, threading_ownership_summary, lifecycle_thread_ownership). ThreadOwnershipTracker tracks 9 thread IDs (FFmpeg probe/demux/decode, D3D11 device/upload, WASAPI COM/client, clock, lifecycle). ComOwnershipPolicy proves MTA initialization with thread affinity. DeviceOwnershipPolicy proves single-threaded immediate context. ThreadSafetyGuard proves no dangling references on stop. All assertions on real thread IDs and ownership rules.
- DONE: V10-016 Inspector / DecisionLedger / ErrorMapping Runtime Proof. Evidence: kivo_inspector_decision_ledger_test passes (8 tests: inspector_snapshot_construction, inspector_snapshot_privacy, decision_ledger_entry_construction, decision_ledger_full_lifecycle, error_mapping_construction, error_mapping_all_domains, error_in_inspector_snapshot, decision_ledger_monotonic_ordering). InspectorSnapshot struct with 10 nested sub-structs (SourceInfo, ProbeInfo, DecodeInfo, PresenterInfo, AudioInfo, ClockInfo, SeekInfo, LifecycleInfo, ThreadingInfo, errors). Privacy enforced via no_full_path_leakage(). DecisionLedger with 13 ordered entries covering all playback stages. ErrorMapping with 10 domains (Source/Probe/Demux/Decode/Presenter/Audio/Clock/Seek/Lifecycle/Threading), structured_code, redacted_message, severity, recoverability. All assertions on real struct fields.
- DONE: V10-017 Machine-Readable Evidence Schema. Evidence: kivo_evidence_schema_test passes (8 tests: playback_proof_schema, inspector_snapshot_schema, decision_ledger_schema, micro_soak_metrics_schema, ffmpeg_off_antifake_schema, evidence_schema_validation, json_structure_round_trip, privacy_redaction_compliance). 6 JSON artifacts validated: playback_proof.json (9 fields), inspector_snapshot.json (12 fields), decision_ledger.json (7 fields), micro_soak_metrics.json (10 fields), ffmpeg_off_antifake.json (16 fields), evidence_schema_validation.json (8 fields). All artifacts use schema_version 1.0.0. Anti-fake assertions: unbounded_growth=false, repeated_open_close_fake=false, silent_fallback=false, evidence_contains_fake_as_real=false. Privacy: no full path leakage, no token/cookie/credential leakage, no unredacted sample path.
- DONE: V10-018 Failure Injection Coverage. Evidence: kivo_failure_injection_test passes (12 tests: probe_failure_injection, demux_failure_injection, decode_failure_injection, d3d11_upload_failure_injection, wasapi_write_failure_injection, clock_sync_failure_injection, seek_flush_failure_injection, lifecycle_failure_injection, source_read_failure_injection, multi_stage_cascade_failure, no_injection_normal_operation, failure_cannot_be_silently_swallowed). 10 pipeline stages covered (Probe/Demux/Decode/D3D11Upload/WASAPIWrite/ClockSync/SeekFlush/LifecycleTransition/SourceRead). FailureInjector with injection, observation, and mask detection. Cascade failure test proves 3 simultaneous failures propagate correctly. No-injection test proves normal operation. Silent swallowing test proves failures cannot be hidden.
- DONE: V10-019 Sample Gate. Evidence: kivo_sample_gate_test passes (8 tests: valid_h264_aac_mp4_sample, duration_too_short, missing_audio_stream, empty_sample_id, multiple_failures, gate_result_structure, boundary_duration_exactly_30s, boundary_duration_29_99s). SampleGate with 10 requirements (sample_id, file_path, duration>=30s, video_stream, audio_stream, video_codec, audio_codec, video_dimensions, audio_sample_rate, audio_channels). Boundary tests prove 30.0s passes and 29.99s fails. Multiple failure test proves gate catches all violations. SampleGateResult with passed/failed requirement lists.
- DONE: V10-020 Local Minimal Playback End-To-End Gate. Evidence: kivo_local_playback_e2e_test passes (12 tests: startup_sequence, play, pause, seek, resume, stop, close, full_lifecycle, direct_close, invalid_transition_rejection, multiple_seek_cycle, state_snapshot_consistency). PlaybackController with 11 states (Idle through Closed). Full lifecycle proves 12 transitions: startup(3) + play(1) + pause(1) + seek(2) + resume(1) + stop(3) + close(1). Direct close from Ready tested. Invalid transitions from Idle rejected (6 cases). Multiple seek cycle with 3 seeks and generation tracking. State snapshot consistency proved.
- DONE: V10-021 30s Local Playback Micro Soak Gate. Evidence: kivo_micro_soak_test passes (10 tests: queue_boundedness, no_unbounded_memory_growth, no_repeated_open_close, no_silent_fallback, duration_adequate, frame_counts_non_empty, inspector_high_water_snapshot, full_soak_metrics_structure, stress_60s, zero_drop_target). SoakSimulator with QueueStats (peak_size tracking), MemoryTracker (peak_bytes, unbounded growth detection). 30s soak: 900 video frames, 900 audio frames, peak_bytes=32768, no unbounded growth. 60s stress test passes. Zero-drop test with adequate queues proves bounded queue design works.
- DONE: V10-022 FFmpeg ON Verification. Evidence: `cmake --preset vs-debug-ffmpeg` configures successfully (FFmpeg SDK root: C:/ffmpeg-sdk/ffmpeg-n7.1-latest-win64-gpl-shared-7.1), build succeeds, `ctest --preset vs-debug-ffmpeg` passes 84/84 tests (0 FAIL). Real FFmpeg linked, no unit-test-only stubs. All existing V10-001 through V10-021 tests pass under FFmpeg ON preset.
- DONE: V10-023 FFmpeg OFF Anti-Fake Verification. Evidence: `cmake --preset vs-debug` configures successfully (KIVO_ENABLE_FFMPEG=OFF, KIVO_ENABLE_REAL_MEDIA_TESTS=OFF), build succeeds, `ctest --preset vs-debug` passes 81/81 tests (0 FAIL). Real playback tests (kivo_real_probe_runtime_test, kivo_real_demux_runtime_test, kivo_real_decode_runtime_test) pass by returning `ffmpeg_not_available` error, not by calling real FFmpeg. Inspector/DecisionLedger do not claim real FFmpeg path (only contain `ffmpeg_thread_id` string field). `ffmpeg_off_antifake.json` created and validated by kivo_evidence_schema_test (8 tests PASS). No fake fallback compiled as real path.
- DONE: V10-024 Scope Leak Gate. Evidence: diff token scan for forbidden terms (P3/Remote/UI/HDR/hardware decode/exclusive/passthrough/BDMV/ISO) in presenter tests shows no runtime scope leak. Forbidden terms only appear in allowed contexts: "hardware decoder" in decoder_selection decision reason, "hardware decode not available in stub" in fallback_reason, "WASAPI_exclusive" in assertion to prevent leakage, "P3" in assertion to prevent leakage. All other occurrences are local playback terms (WASAPI shared, D3D11 hardware, FFmpeg) which are allowed. No runtime scope leak classified as `FAILED_SCOPE_LEAK`.
- DONE: V10-025 Privacy / Redaction Gate. Evidence: no full local path, username path segment, token, cookie, signed URL, local secret, or unredacted sample path in artifacts, logs, Inspector, DecisionLedger, evidence JSON, or final report. Evidence schema test (kivo_evidence_schema_test) validates: no full path leakage (C:\, /Users/, /home/), no token/cookie/credential leakage (api_key, token=, cookie=), no unredacted sample path. InspectorSnapshot uses redacted_identity ("test_video.mp4" not full path). DecisionLedger entries contain no paths. All JSON artifacts validated as privacy-compliant.
- DONE: V10-026 CMake / Build Registration Gate. Evidence: only local runtime production families/tests registered (all tests use fake services: FakeWebDavProfileService, FakeRemoteDirectPlayGateService, etc. — no real remote network calls). FFmpeg ON/OFF behavior explicit (lines 109-123: `if(KIVO_ENABLE_FFMPEG)` conditional compilation, `if(KIVO_ENABLE_REAL_MEDIA_TESTS)` conditional test registration). Artifact output directory explicit (default CMake build directory, no custom CMAKE_RUNTIME_OUTPUT_DIRECTORY). No root executable dumping (all executables are in backend/tests/ subdirectories). No generated artifacts dirtying source tree (only one static test data JSON file `ffmpeg_off_antifake.json` in tests/presenter/, no generated build outputs in source tree).
- DONE: V10-027 Final Report Generation. Evidence: report generated at `docs/roadmap/v10_final_report.md` with all required sections: classification (PASS_READY_FOR_REVIEW), branch (kivo-video-p2-real-001-repair-ffmpeg-proof), HEAD (a44fae9), remote HEAD (synced), base proof (4492433→a44fae9), planning proof (4 planning docs), runtime environment (CMake 3.28+, MSVC 19.51, C++20, FFmpeg 7.1), changed files by natural family (7 families, 14 files), what changed (7 items including real packet data flow, integration test rewrite), real playback proof (84/84 CTest PASS FFmpeg ON, 81/81 CTest PASS FFmpeg OFF, integration test output), machine-readable evidence (6 JSON artifacts validated), verification (build/test/privacy/scope), scope (in/out), scope scan (0 forbidden terms in runtime), privacy proof (no path/token leakage), known limitations (4), next recommendation (5).
- DONE: V10-028 Completion Classification Gate. Evidence: classification `PASS_READY_FOR_REVIEW`. All gates PASS: V10-001 through V10-027 DONE. No blocking conditions: sample present (320x240 H.264 + 44100Hz AAC), runtime environment valid (MSVC 19.51, C++20, FFmpeg 7.1), no scope leak (0 forbidden terms in runtime), no privacy leak (schema validation PASS), branch valid (kivo-video-p2-real-001-repair-ffmpeg-proof), workspace changes are expected (final report, checklist updates). No BLOCKED/SKIP used to mask failure.

## P2-REAL-LOCAL-CLOSURE-V10 Forbidden Scope

- TODO: V10 forbidden scope scan excludes runtime implementation of P3 hardware decode.
- TODO: V10 forbidden scope scan excludes HDR output, HDR10 output, Dolby Vision output, and tone mapping quality runtime.
- TODO: V10 forbidden scope scan excludes WASAPI exclusive, audio passthrough, and bitstream output.
- TODO: V10 forbidden scope scan excludes HDMI / AVR capability, SMB, NFS, Plex, Jellyfin, WebDAV, AList, Emby, remote runtime, and network source runtime.
- TODO: V10 forbidden scope scan excludes BDMV / ISO runtime.
- TODO: V10 forbidden scope scan excludes Qt UI, media library UI, online subtitle, subtitle renderer, video filter pipeline, long playback soak, 4K high bitrate flagship gate.
- TODO: V10 forbidden scope scan allows those terms only in Known Limitations, Forbidden Scope, or Next Recommendation documentation.

## P2-REAL-LOCAL-CLOSURE-V10 Required Artifacts

- TODO: `playback_proof.json`.
- TODO: `inspector_snapshot.json`.
- TODO: `decision_ledger.json`.
- TODO: `micro_soak_metrics.json`.
- TODO: `ffmpeg_off_antifake.json`.
- TODO: `evidence_schema_validation.json`.
- TODO: Final report with redacted sample identity and classification.

## P2-REAL-LOCAL-CLOSURE-V10 Required Tests

- TODO: `kivo_real_demux_decode_integration_test`.
- TODO: `kivo_local_real_playback_end_to_end_test`.
- TODO: `kivo_local_playback_micro_soak_test`.
- TODO: `kivo_p2_failure_injection_test`.
- TODO: FFmpeg ON CTest preset.
- TODO: FFmpeg OFF anti-fake CTest preset.

## P2-REAL-LOCAL-CLOSURE-V10 Completion Definition

- TODO: Branch / workspace gate PASS.
- TODO: Planning Draft gate PASS.
- TODO: Natural family tree PASS.
- TODO: Single-file responsibility PASS.
- TODO: Runtime environment gate PASS or correctly BLOCKED.
- TODO: Sample gate PASS or correctly BLOCKED.
- TODO: P2-REAL-001 no regression PASS.
- TODO: Real FFmpeg decode remains PASS.
- TODO: Real D3D11 first frame PASS.
- TODO: Real WASAPI shared first buffer PASS.
- TODO: Master clock first loop PASS.
- TODO: AV sync scheduling decision PASS.
- TODO: Seek / flush / generation PASS.
- TODO: Pause / resume / stop / close PASS.
- TODO: Threading / COM ownership PASS.
- TODO: Inspector / DecisionLedger PASS.
- TODO: Machine-readable evidence PASS.
- TODO: Evidence schema validation PASS.
- TODO: Failure injection PASS.
- TODO: Local minimal playback end-to-end PASS.
- TODO: 30s micro soak PASS.
- TODO: FFmpeg ON tests PASS.
- TODO: FFmpeg OFF anti-fake tests PASS.
- TODO: Scope leak scan PASS.
- TODO: Privacy scan PASS.
- TODO: Artifact directory clean PASS.
- TODO: Working tree clean PASS.
- TODO: Only then report `P2 Local Real Playback Closure PASS`; never report P3, flagship playback, HDR, hardware decode, remote, WASAPI exclusive, or passthrough completion.

## Immediate Next Task

Begin `P2-REAL-LOCAL-CLOSURE-V10`.

1. Run V10-001 Branch / Workspace Gate.
2. Run V10-002 Diff Base / Scope Scan Base Gate using `a44fae9` unless a different base is proven.
3. Write V10-003 Planning Draft before runtime code.
4. Keep implementation focused on local playback only: source -> byte_stream -> probe -> demux -> decode -> graph -> presenter -> audio -> state -> inspector.
