# Kivo Video Backend Task Checklist

Updated: 2026-06-25 11:50 (P2-049 DONE)

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
- TODO: P2-050 Test Oracle / Reproducibility Skeleton. Evidence target: hermetic fake + manual hardware pair policy.
- TODO: P2-051 Media Parser Security / Fuzz Skeleton. Evidence target: parser security core, quarantine, fuzz shell.
- TODO: P2-052 Build / Signing / Symbol / Update Policy Skeleton. Evidence target: build/signing/symbol/update policy docs.
- TODO: P2-053 Recovery / Fallback Runtime. Evidence target: recovery matrix implementation and failure budget.
- TODO: P2-054 Release Readiness Matrix. Evidence target: P2 release readiness matrix.
- TODO: P2-055 Full Integration Gate. Evidence target: P2.1 / P2.1.5 / P2.2 / P2.3 pass evidence.

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

## Immediate Next Task

P2-001A/B/C through P2-049 DONE. Proceed to P2-050 through P2-055.

Next: P2-050 Test Oracle / Reproducibility Skeleton.
