# Kivo Video Backend Task Checklist

Updated: 2026-06-25 00:25

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

## Immediate Next Task

All adapter boundary tasks completed:
- Adapter boundary directory rules verification in governance script.
- No non-entry implementation files added to root verification in governance script.
- `backend/tests/adapter_boundary/adapter_boundary_test.cpp`
- Governance checks for adapter boundary directory rules.
- `cmake --preset ninja-debug`
- `cmake --build --preset ninja-debug`
- `ctest --preset ninja-debug --output-on-failure`

P1 backend implementation is now complete. All governance gates pass.
