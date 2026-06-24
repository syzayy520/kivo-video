from pathlib import Path
import re
import sys


ROOT = Path(__file__).resolve().parents[3]
BACKEND = ROOT / "backend"

REQUIRED_DOCS = [
    "docs/p0/core-architecture/core_freeze_spec.md",
    "docs/p0/foundation/contract_registry.md",
    "docs/p0/session-ledger/playback_session_model.md",
    "docs/p0/session-ledger/decision_ledger_model.md",
    "docs/p0/playback-plan/playback_plan_schema.md",
    "docs/p0/capability-policy/capability_join_result.md",
    "docs/p0/source-remote-object/source_identity_and_remote_object.md",
    "docs/p0/media-experience/media_experience_snapshot.md",
    "docs/p0/color-hdr/color_output_matrix.md",
    "docs/p0/windows-presenter/windows_hdr_output_contract.md",
    "docs/p0/windows-presenter/presenter_and_frame_pacing_contract.md",
    "docs/p0/governance-ci/governance_ci_gates.md",
    "docs/p0/audio-subtitle/subtitle_render_plan.md",
    "docs/p0/audio-subtitle/audio_output_decision.md",
    "docs/p0/diagnostic-inspector/playback_inspector_snapshot.md",
    "docs/p0/diagnostic-inspector/diagnostic_bundle_manifest.md",
    "docs/roadmap/p1_engine_api_boundary.md",
    "docs/roadmap/p1_session_ledger_store.md",
    "docs/roadmap/p1_replay_manifest.md",
    "docs/roadmap/p1_playback_state_machine.md",
    "docs/roadmap/p1_graph_runtime.md",
    "docs/roadmap/p1_media_pipeline_contracts.md",
    "docs/roadmap/p1_color_hdr_presenter_services.md",
    "docs/roadmap/p1_audio_subtitle_services.md",
    # P2 design docs
    "docs/p2_real_playback/architecture.md",
    "docs/p2_real_playback/family_tree.md",
    "docs/p2_real_playback/minimal_executable_architecture_slice.md",
    "docs/p2_real_playback/governance_gate_skeleton.md",
    "docs/p2_real_playback/future_contract_placeholders.md",
]

REQUIRED_CONTRACTS = [
    "EngineCommand",
    "EngineEvent",
    "PlaybackSessionSnapshot",
    "PlaybackSessionAggregate",
    "DecisionLedgerEntry",
    "ReplayManifest",
    "PlaybackPlanSchema",
    "CapabilityJoinResult",
    "PolicyDecision",
    "ComplianceGateResult",
    "FeatureGateSnapshot",
    "ResourceBudgetResult",
    "SourcePlayableObject",
    "RemoteObjectIdentity",
    "StreamInventory",
    "MediaExperienceSnapshot",
    "ColorOutputMatrix",
    "WindowsHdrOutputContract",
    "VideoPresenterContract",
    "SubtitleRenderPlan",
    "AudioOutputDecision",
    "PlaybackInspectorSnapshot",
    "DiagnosticBundleManifest",
    "EngineStateSnapshot",
    "EngineError",
]

ALLOWED_ROOT_FILES = {
    ".gitignore",
    "AGENTS.md",
    "CMakeLists.txt",
    "CMakePresets.json",
    "README.md",
}

FORBIDDEN_CORE_PATTERNS = [
    r"#\s*include\s*[<\"]windows\.h[>\"]",
    r"#\s*include\s*[<\"].*d3d.*[>\"]",
    r"#\s*include\s*[<\"].*dxgi.*[>\"]",
    r"#\s*include\s*[<\"].*wasapi.*[>\"]",
    r"#\s*include\s*[<\"].*libav.*[>\"]",
    r"\bAVFormatContext\b",
    r"\bAVCodecContext\b",
    r"\bAVFrame\b",
    r"\bAVPacket\b",
]


def fail(message: str) -> None:
    print(f"FAIL: {message}")
    sys.exit(1)


def read(path: Path) -> str:
    return path.read_text(encoding="utf-8")


def check_docs() -> None:
    for rel in REQUIRED_DOCS:
        if not (ROOT / rel).is_file():
            fail(f"missing P0 doc: {rel}")


def check_root_not_flat() -> None:
    for path in ROOT.iterdir():
        if path.is_file() and path.name not in ALLOWED_ROOT_FILES:
            fail(f"unexpected root-level file: {path.name}")


def check_contract_registry() -> None:
    registry_text = read(BACKEND / "src/foundation/contract_registry.cpp")
    for contract in REQUIRED_CONTRACTS:
        if f'"{contract}"' not in registry_text:
            fail(f"contract not registered: {contract}")


def check_schema_metadata() -> None:
    include_root = BACKEND / "include/kivo/cinema_engine"
    header_text = "\n".join(read(path) for path in include_root.rglob("*.hpp"))
    for contract in REQUIRED_CONTRACTS:
        pattern = rf"struct\s+{re.escape(contract)}\s*\{{[\s\S]*?ContractMetadata\s+metadata"
        if not re.search(pattern, header_text):
            fail(f"contract has no ContractMetadata metadata field: {contract}")


def check_forbidden_includes() -> None:
    include_root = BACKEND / "include/kivo/cinema_engine"
    source_root = BACKEND / "src"
    for path in list(include_root.rglob("*.[hc]pp")) + list(source_root.rglob("*.[hc]pp")):
      text = read(path)
      for pattern in FORBIDDEN_CORE_PATTERNS:
          if re.search(pattern, text, flags=re.IGNORECASE):
              fail(f"forbidden backend adapter type leaked into core: {path}")


def check_fake_replay_contracts() -> None:
    text = read(BACKEND / "src/fake_backend/fake_session_scenario.cpp")
    required_terms = [
        "sequence_number",
        "generation",
        "feature_gate_snapshot_ref",
        "remote_object_identity_ref",
        "compliance_gate_ref",
        "fallback_chain",
        "recovery_actions",
        "excluded_secret_kinds",
    ]
    for term in required_terms:
        if term not in text:
            fail(f"fake session missing replay-critical term: {term}")


def check_redaction_gate_model() -> None:
    sample = "token=secret cookie=session signed_url=https://example.test/file credential=plain"
    redacted = re.sub(r"(token|cookie|signed_url|credential)=[^\s&]+", r"\1=<redacted>", sample)
    for leaked in ["secret", "session", "https://example.test", "plain"]:
        if leaked in redacted:
            fail(f"diagnostic redaction model leaked {leaked}")


def check_invariant_gate_model() -> None:
    violations = []
    state = "Playing"
    has_active_session = False
    has_graph_binding = False
    if state == "Playing" and not has_active_session:
        violations.append("PLAYING_REQUIRES_ACTIVE_SESSION")
    if state == "Playing" and not has_graph_binding:
        violations.append("PLAYING_REQUIRES_GRAPH_BINDING")
    if violations != ["PLAYING_REQUIRES_ACTIVE_SESSION", "PLAYING_REQUIRES_GRAPH_BINDING"]:
        fail("playback invariant model did not catch broken Playing state")


def check_gate_sources_exist() -> None:
    required = [
        "backend/include/kivo/cinema_engine/playback_core/playback_invariant.hpp",
        "backend/src/playback_core/playback_invariant.cpp",
        "backend/include/kivo/cinema_engine/diagnostic_core/diagnostic_redaction_gate.hpp",
        "backend/src/diagnostic_core/diagnostic_redaction_gate.cpp",
        "backend/tests/contract_gates/playback_invariant_test.cpp",
        "backend/tests/contract_gates/diagnostic_redaction_gate_test.cpp",
    ]
    for rel in required:
        if not (ROOT / rel).is_file():
            fail(f"missing gate source: {rel}")


def check_engine_api_boundary_sources() -> None:
    required = [
        "backend/include/kivo/cinema_engine/engine_api/command_queue.hpp",
        "backend/src/engine_api/command_queue.cpp",
        "backend/include/kivo/cinema_engine/engine_api/event_bus.hpp",
        "backend/src/engine_api/event_bus.cpp",
        "backend/include/kivo/cinema_engine/engine_api/state_snapshot_store.hpp",
        "backend/src/engine_api/state_snapshot_store.cpp",
        "backend/include/kivo/cinema_engine/engine_api/cancellation_token.hpp",
        "backend/src/engine_api/cancellation_token.cpp",
        "backend/tests/engine_api/engine_api_boundary_test.cpp",
    ]
    for rel in required:
        if not (ROOT / rel).is_file():
            fail(f"missing Engine API boundary source: {rel}")

    cmake_text = read(BACKEND / "CMakeLists.txt")
    for rel in [
        "src/engine_api/command_queue.cpp",
        "src/engine_api/event_bus.cpp",
        "src/engine_api/state_snapshot_store.cpp",
        "src/engine_api/cancellation_token.cpp",
        "tests/engine_api/engine_api_boundary_test.cpp",
    ]:
        if rel not in cmake_text:
            fail(f"Engine API source not wired into CMake: {rel}")

    command_text = read(BACKEND / "include/kivo/cinema_engine/engine_api/command_queue.hpp")
    event_text = read(BACKEND / "include/kivo/cinema_engine/engine_api/event_bus.hpp")
    snapshot_text = read(BACKEND / "include/kivo/cinema_engine/engine_api/state_snapshot_store.hpp")
    cancellation_text = read(BACKEND / "include/kivo/cinema_engine/engine_api/cancellation_token.hpp")
    required_terms = {
        "CommandQueue": command_text,
        "submit": command_text,
        "try_pop": command_text,
        "EventBus": event_text,
        "subscribe": event_text,
        "poll": event_text,
        "StateSnapshotStore": snapshot_text,
        "publish": snapshot_text,
        "latest": snapshot_text,
        "CancellationToken": cancellation_text,
        "request_cancellation": cancellation_text,
        "is_cancellation_requested": cancellation_text,
    }
    for term, text in required_terms.items():
        if term not in text:
            fail(f"Engine API boundary term missing: {term}")


def check_session_ledger_sources() -> None:
    required = [
        "backend/include/kivo/cinema_engine/session_core/session_store.hpp",
        "backend/src/session_core/session_store.cpp",
        "backend/include/kivo/cinema_engine/decision_ledger_core/decision_ledger_store.hpp",
        "backend/src/decision_ledger_core/decision_ledger_store.cpp",
        "backend/include/kivo/cinema_engine/decision_ledger_core/replay_manifest.hpp",
        "backend/src/decision_ledger_core/replay_manifest.cpp",
        "backend/tests/session_ledger/session_ledger_store_test.cpp",
    ]
    for rel in required:
        if not (ROOT / rel).is_file():
            fail(f"missing Session/Ledger source: {rel}")

    cmake_text = read(BACKEND / "CMakeLists.txt")
    for rel in [
        "src/session_core/session_store.cpp",
        "src/decision_ledger_core/decision_ledger_store.cpp",
        "src/decision_ledger_core/replay_manifest.cpp",
        "tests/session_ledger/session_ledger_store_test.cpp",
    ]:
        if rel not in cmake_text:
            fail(f"Session/Ledger source not wired into CMake: {rel}")

    session_text = read(BACKEND / "include/kivo/cinema_engine/session_core/session_store.hpp")
    ledger_text = read(BACKEND / "include/kivo/cinema_engine/decision_ledger_core/decision_ledger_store.hpp")
    for term in ["create", "close", "find", "export_all"]:
        if term not in session_text:
            fail(f"SessionStore missing operation: {term}")
    for term in ["append", "entries_for_session", "find_entry", "export_all"]:
        if term not in ledger_text:
            fail(f"DecisionLedgerStore missing operation: {term}")

    replay_text = read(BACKEND / "include/kivo/cinema_engine/decision_ledger_core/replay_manifest.hpp")
    for term in ["ReplayManifest", "ordered_entry_ids", "replayable", "replay_blocker", "build_replay_manifest"]:
        if term not in replay_text:
            fail(f"ReplayManifest missing term: {term}")

    ledger_impl = read(BACKEND / "src/decision_ledger_core/decision_ledger_store.cpp")
    for term in ["sequence_number == 0", "!entry.generation.has_value()", "entry.sequence_number <= same_session_latest->sequence_number"]:
        if term not in ledger_impl:
            fail(f"DecisionLedgerStore missing replay guard: {term}")


def check_fake_engine_api_scenario() -> None:
    required = [
        "backend/include/kivo/cinema_engine/fake_backend/fake_engine_api_scenario.hpp",
        "backend/src/fake_backend/fake_engine_api_scenario.cpp",
        "backend/tests/fake_backend/fake_engine_api_scenario_test.cpp",
    ]
    for rel in required:
        if not (ROOT / rel).is_file():
            fail(f"missing fake Engine API scenario source: {rel}")

    cmake_text = read(BACKEND / "CMakeLists.txt")
    for rel in [
        "src/fake_backend/fake_engine_api_scenario.cpp",
        "tests/fake_backend/fake_engine_api_scenario_test.cpp",
    ]:
        if rel not in cmake_text:
            fail(f"fake Engine API scenario not wired into CMake: {rel}")

    scenario_text = read(BACKEND / "src/fake_backend/fake_engine_api_scenario.cpp")
    for term in ["CommandQueue", "EventBus", "StateSnapshotStore", "SessionStore", "DecisionLedgerStore", "build_replay_manifest"]:
        if term not in scenario_text:
            fail(f"fake Engine API scenario missing term: {term}")


def check_playback_state_machine_sources() -> None:
    required = [
        "backend/include/kivo/cinema_engine/playback_core/fake_playback_state_machine.hpp",
        "backend/src/playback_core/fake_playback_state_machine.cpp",
        "backend/tests/playback_core/fake_playback_state_machine_test.cpp",
    ]
    for rel in required:
        if not (ROOT / rel).is_file():
            fail(f"missing PlaybackCore state machine source: {rel}")

    cmake_text = read(BACKEND / "CMakeLists.txt")
    for rel in [
        "src/playback_core/fake_playback_state_machine.cpp",
        "tests/playback_core/fake_playback_state_machine_test.cpp",
    ]:
        if rel not in cmake_text:
            fail(f"PlaybackCore state machine not wired into CMake: {rel}")

    state_machine_text = read(BACKEND / "src/playback_core/fake_playback_state_machine.cpp")
    for term in ["open", "bind_graph", "play", "pause", "seek", "recover", "close", "check_playback_invariants"]:
        if term not in state_machine_text:
            fail(f"PlaybackCore state machine missing term: {term}")


def check_graph_runtime_sources() -> None:
    required = [
        "backend/include/kivo/cinema_engine/graph_core/graph_invariant.hpp",
        "backend/src/graph_core/graph_invariant.cpp",
        "backend/include/kivo/cinema_engine/graph_core/fake_graph_runtime.hpp",
        "backend/src/graph_core/fake_graph_runtime.cpp",
        "backend/tests/graph_core/fake_graph_runtime_test.cpp",
    ]
    for rel in required:
        if not (ROOT / rel).is_file():
            fail(f"missing GraphCore source: {rel}")

    cmake_text = read(BACKEND / "CMakeLists.txt")
    for rel in [
        "src/graph_core/graph_invariant.cpp",
        "src/graph_core/fake_graph_runtime.cpp",
        "tests/graph_core/fake_graph_runtime_test.cpp",
    ]:
        if rel not in cmake_text:
            fail(f"GraphCore source not wired into CMake: {rel}")

    graph_text = read(BACKEND / "src/graph_core/fake_graph_runtime.cpp")
    for term in ["bind", "add_node", "add_edge", "set_seek_barrier", "set_backpressure", "flush", "stop", "check_graph_invariants"]:
        if term not in graph_text:
            fail(f"GraphCore runtime missing term: {term}")

    invariant_text = read(BACKEND / "src/graph_core/graph_invariant.cpp")
    for term in ["GRAPH_EDGE_REQUIRES_CAPACITY", "GRAPH_NODE_REQUIRES_LIFECYCLE", "GRAPH_STOP_FORBIDS_FRAME", "GRAPH_FLUSH_REQUIRES_GENERATION"]:
        if term not in invariant_text:
            fail(f"GraphCore invariant missing term: {term}")


def check_media_pipeline_contract_sources() -> None:
    required = [
        "backend/include/kivo/cinema_engine/network_cache_core/fake_cache_reader.hpp",
        "backend/src/network_cache_core/fake_cache_reader.cpp",
        "backend/include/kivo/cinema_engine/demux_core/fake_stream_probe.hpp",
        "backend/src/demux_core/fake_stream_probe.cpp",
        "backend/include/kivo/cinema_engine/decode_core/fake_decode_path.hpp",
        "backend/src/decode_core/fake_decode_path.cpp",
        "backend/tests/media_pipeline/media_pipeline_contract_test.cpp",
    ]
    for rel in required:
        if not (ROOT / rel).is_file():
            fail(f"missing media pipeline contract source: {rel}")

    cmake_text = read(BACKEND / "CMakeLists.txt")
    for rel in [
        "src/network_cache_core/fake_cache_reader.cpp",
        "src/demux_core/fake_stream_probe.cpp",
        "src/decode_core/fake_decode_path.cpp",
        "tests/media_pipeline/media_pipeline_contract_test.cpp",
    ]:
        if rel not in cmake_text:
            fail(f"media pipeline source not wired into CMake: {rel}")

    cache_text = read(BACKEND / "src/network_cache_core/fake_cache_reader.cpp")
    for term in ["remote_object_identity_ref", "remote_object_identity_id", "cache_extent_id", "readable_bytes"]:
        if term not in cache_text:
            fail(f"fake cache reader missing term: {term}")

    demux_text = read(BACKEND / "src/demux_core/fake_stream_probe.cpp")
    for term in ["StreamInventory", "video_streams", "audio_streams", "subtitle_streams", "hdr_metadata", "dovi_metadata"]:
        if term not in demux_text:
            fail(f"fake stream probe missing term: {term}")

    decode_text = read(BACKEND / "src/decode_core/fake_decode_path.cpp")
    for term in ["FakeDecodePath", "hardware_preferred", "software_fallback_available", "HW_DECODE_TO_SW_DECODE"]:
        if term not in decode_text:
            fail(f"fake decode path missing term: {term}")


def check_color_hdr_presenter_sources() -> None:
    required = [
        "backend/include/kivo/cinema_engine/color_science_core/fake_color_output_service.hpp",
        "backend/src/color_science_core/fake_color_output_service.cpp",
        "backend/include/kivo/cinema_engine/windows_hdr_core/fake_windows_hdr_service.hpp",
        "backend/src/windows_hdr_core/fake_windows_hdr_service.cpp",
        "backend/include/kivo/cinema_engine/video_render_core/fake_presenter_service.hpp",
        "backend/src/video_render_core/fake_presenter_service.cpp",
        "backend/tests/color_hdr_presenter/color_hdr_presenter_contract_test.cpp",
    ]
    for rel in required:
        if not (ROOT / rel).is_file():
            fail(f"missing Color/HDR/Presenter source: {rel}")

    cmake_text = read(BACKEND / "CMakeLists.txt")
    for rel in [
        "src/color_science_core/fake_color_output_service.cpp",
        "src/windows_hdr_core/fake_windows_hdr_service.cpp",
        "src/video_render_core/fake_presenter_service.cpp",
        "tests/color_hdr_presenter/color_hdr_presenter_contract_test.cpp",
    ]:
        if rel not in cmake_text:
            fail(f"Color/HDR/Presenter source not wired into CMake: {rel}")

    color_text = read(BACKEND / "src/color_science_core/fake_color_output_service.cpp")
    for term in ["ComplianceGateResult", "claim_level", "compliance_gate_ref", "DV_aware_experimental_mapping"]:
        if term not in color_text:
            fail(f"ColorOutput service missing term: {term}")

    hdr_text = read(BACKEND / "src/windows_hdr_core/fake_windows_hdr_service.cpp")
    for term in ["ColorOutputMatrix", "color_output_matrix_ref", "hdr_enabled", "target_display_class"]:
        if term not in hdr_text:
            fail(f"Windows HDR service missing term: {term}")

    presenter_text = read(BACKEND / "src/video_render_core/fake_presenter_service.cpp")
    for term in ["WindowsHdrOutputContract", "ColorOutputMatrix", "hdr_output_ref", "color_output_matrix_ref", "exact-24p"]:
        if term not in presenter_text:
            fail(f"Presenter service missing term: {term}")


def check_audio_subtitle_sources() -> None:
    required = [
        "backend/include/kivo/cinema_engine/subtitle_core/fake_subtitle_service.hpp",
        "backend/src/subtitle_core/fake_subtitle_service.cpp",
        "backend/include/kivo/cinema_engine/audio_core/fake_audio_service.hpp",
        "backend/src/audio_core/fake_audio_service.cpp",
        "backend/tests/audio_subtitle/audio_subtitle_contract_test.cpp",
    ]
    for rel in required:
        if not (ROOT / rel).is_file():
            fail(f"missing Audio/Subtitle source: {rel}")

    cmake_text = read(BACKEND / "CMakeLists.txt")
    for rel in [
        "src/subtitle_core/fake_subtitle_service.cpp",
        "src/audio_core/fake_audio_service.cpp",
        "tests/audio_subtitle/audio_subtitle_contract_test.cpp",
    ]:
        if rel not in cmake_text:
            fail(f"Audio/Subtitle source not wired into CMake: {rel}")

    subtitle_text = read(BACKEND / "src/subtitle_core/fake_subtitle_service.cpp")
    for term in ["embedded_font_policy", "hdr_luminance_policy", "fallback-with-reason", "tone-map-subtitle-luma"]:
        if term not in subtitle_text:
            fail(f"Subtitle service missing term: {term}")

    audio_text = read(BACKEND / "src/audio_core/fake_audio_service.cpp")
    for term in ["ComplianceGateResult", "passthrough_allowed", "pcm_decode", "fallback_reason", "user_visible_hint"]:
        if term not in audio_text:
            fail(f"Audio service missing term: {term}")


def check_inspector_diagnostic_sources() -> None:
    required = [
        "backend/include/kivo/cinema_engine/playback_inspector_core/fake_inspector_service.hpp",
        "backend/src/playback_inspector_core/fake_inspector_service.cpp",
        "backend/include/kivo/cinema_engine/diagnostic_core/fake_diagnostic_bundle_export.hpp",
        "backend/src/diagnostic_core/fake_diagnostic_bundle_export.cpp",
        "backend/tests/inspector_diagnostic/inspector_diagnostic_contract_test.cpp",
    ]
    for rel in required:
        if not (ROOT / rel).is_file():
            fail(f"missing Inspector/Diagnostic source: {rel}")

    cmake_text = read(BACKEND / "CMakeLists.txt")
    for rel in [
        "src/playback_inspector_core/fake_inspector_service.cpp",
        "src/diagnostic_core/fake_diagnostic_bundle_export.cpp",
        "tests/inspector_diagnostic/inspector_diagnostic_contract_test.cpp",
    ]:
        if rel not in cmake_text:
            fail(f"Inspector/Diagnostic source not wired into CMake: {rel}")

    inspector_text = read(BACKEND / "src/playback_inspector_core/fake_inspector_service.cpp")
    for term in ["PlaybackInspectorSnapshot", "inspector_snapshot_id", "user_summary", "redaction_state", "actions_available"]:
        if term not in inspector_text:
            fail(f"Inspector service missing term: {term}")

    diagnostic_text = read(BACKEND / "src/diagnostic_core/fake_diagnostic_bundle_export.cpp")
    for term in ["DiagnosticBundleManifest", "diagnostic_bundle_id", "redaction_profile", "excluded_secret_kinds", "export_requires_consent"]:
        if term not in diagnostic_text:
            fail(f"Diagnostic service missing term: {term}")


def check_policy_compliance_resource_sources() -> None:
    required = [
        "backend/include/kivo/cinema_engine/resource_core/fake_resource_budget_service.hpp",
        "backend/src/resource_core/fake_resource_budget_service.cpp",
        "backend/include/kivo/cinema_engine/policy_core/fake_policy_service.hpp",
        "backend/src/policy_core/fake_policy_service.cpp",
        "backend/include/kivo/cinema_engine/compliance_core/fake_compliance_service.hpp",
        "backend/src/compliance_core/fake_compliance_service.cpp",
        "backend/include/kivo/cinema_engine/feature_gate_core/fake_feature_gate_service.hpp",
        "backend/src/feature_gate_core/fake_feature_gate_service.cpp",
        "backend/tests/policy_compliance_resource/policy_compliance_resource_contract_test.cpp",
    ]
    for rel in required:
        if not (ROOT / rel).is_file():
            fail(f"missing Policy/Compliance/Resource source: {rel}")

    cmake_text = read(BACKEND / "CMakeLists.txt")
    for rel in [
        "src/resource_core/fake_resource_budget_service.cpp",
        "src/policy_core/fake_policy_service.cpp",
        "src/compliance_core/fake_compliance_service.cpp",
        "src/feature_gate_core/fake_feature_gate_service.cpp",
        "tests/policy_compliance_resource/policy_compliance_resource_contract_test.cpp",
    ]:
        if rel not in cmake_text:
            fail(f"Policy/Compliance/Resource source not wired into CMake: {rel}")

    resource_text = read(BACKEND / "src/resource_core/fake_resource_budget_service.cpp")
    for term in ["ResourceBudgetResult", "resource_budget_id", "cpu_budget", "gpu_budget", "memory_budget", "pressure_risks"]:
        if term not in resource_text:
            fail(f"Resource service missing term: {term}")

    policy_text = read(BACKEND / "src/policy_core/fake_policy_service.cpp")
    for term in ["PolicyDecision", "policy_decision_id", "decision", "reasons", "user_visible_summary"]:
        if term not in policy_text:
            fail(f"Policy service missing term: {term}")

    compliance_text = read(BACKEND / "src/compliance_core/fake_compliance_service.cpp")
    for term in ["ComplianceGateResult", "compliance_gate_id", "claim_level", "allowed_claims", "forbidden_claim_reasons"]:
        if term not in compliance_text:
            fail(f"Compliance service missing term: {term}")

    feature_gate_text = read(BACKEND / "src/feature_gate_core/fake_feature_gate_service.cpp")
    for term in ["FeatureGateSnapshot", "feature_gate_snapshot_id", "enabled_features", "disabled_features", "experimental_features"]:
        if term not in feature_gate_text:
            fail(f"FeatureGate service missing term: {term}")


def check_adapter_boundary_sources() -> None:
    required = [
        "backend/include/kivo/cinema_engine/adapter_boundary/adapter_boundary_contract.hpp",
        "backend/src/adapter_boundary/adapter_boundary_contract.cpp",
        "backend/tests/adapter_boundary/adapter_boundary_test.cpp",
    ]
    for rel in required:
        if not (ROOT / rel).is_file():
            fail(f"missing Adapter Boundary source: {rel}")

    cmake_text = read(BACKEND / "CMakeLists.txt")
    for rel in [
        "src/adapter_boundary/adapter_boundary_contract.cpp",
        "tests/adapter_boundary/adapter_boundary_test.cpp",
    ]:
        if rel not in cmake_text:
            fail(f"Adapter Boundary source not wired into CMake: {rel}")

    header_text = read(BACKEND / "include/kivo/cinema_engine/adapter_boundary/adapter_boundary_contract.hpp")
    for term in ["AdapterBoundaryContract", "adapter_boundary_id", "adapter_type", "supported_operations", "required_capabilities", "boundary_valid"]:
        if term not in header_text:
            fail(f"Adapter Boundary header missing term: {term}")

    source_text = read(BACKEND / "src/adapter_boundary/adapter_boundary_contract.cpp")
    for term in ["AdapterBoundaryContract", "metadata", "boundary_valid"]:
        if term not in source_text:
            fail(f"Adapter Boundary source missing term: {term}")


def check_adapter_boundary_directory_rules() -> None:
    # Verify that adapter-related code is in proper directories
    # Adapter boundary should be in dedicated directories, not mixed with core
    
    # Check that adapter boundary directory exists
    adapter_boundary_dir = BACKEND / "include/kivo/cinema_engine/adapter_boundary"
    if not adapter_boundary_dir.is_dir():
        fail("missing adapter_boundary directory in include/kivo/cinema_engine/")
    
    # Check that adapter boundary source directory exists
    adapter_boundary_src_dir = BACKEND / "src/adapter_boundary"
    if not adapter_boundary_src_dir.is_dir():
        fail("missing adapter_boundary directory in src/")
    
    # Check that adapter boundary test directory exists
    adapter_boundary_test_dir = BACKEND / "tests/adapter_boundary"
    if not adapter_boundary_test_dir.is_dir():
        fail("missing adapter_boundary directory in tests/")
    
    # Verify no adapter-specific patterns in core directories
    # These patterns indicate adapter boundary violations
    adapter_patterns = [
        r"\bWASAPI\b",
        r"\bwasapi\b",
        r"\bD3D11\b",
        r"\bd3d11\b",
        r"\bDXGI\b",
        r"\bdxgi\b",
        r"\bAVFormatContext\b",
        r"\bAVCodecContext\b",
        r"\bAVFrame\b",
        r"\bAVPacket\b",
    ]
    
    # Check core include directories for adapter violations
    core_include_dirs = [
        BACKEND / "include/kivo/cinema_engine/playback_core",
        BACKEND / "include/kivo/cinema_engine/engine_api",
        BACKEND / "include/kivo/cinema_engine/session_core",
        BACKEND / "include/kivo/cinema_engine/decision_ledger_core",
        BACKEND / "include/kivo/cinema_engine/graph_core",
        BACKEND / "include/kivo/cinema_engine/network_cache_core",
        BACKEND / "include/kivo/cinema_engine/demux_core",
        BACKEND / "include/kivo/cinema_engine/decode_core",
        BACKEND / "include/kivo/cinema_engine/color_science_core",
        BACKEND / "include/kivo/cinema_engine/windows_hdr_core",
        BACKEND / "include/kivo/cinema_engine/video_render_core",
        BACKEND / "include/kivo/cinema_engine/subtitle_core",
        BACKEND / "include/kivo/cinema_engine/audio_core",
        BACKEND / "include/kivo/cinema_engine/playback_inspector_core",
        BACKEND / "include/kivo/cinema_engine/diagnostic_core",
        BACKEND / "include/kivo/cinema_engine/resource_core",
        BACKEND / "include/kivo/cinema_engine/policy_core",
        BACKEND / "include/kivo/cinema_engine/compliance_core",
        BACKEND / "include/kivo/cinema_engine/feature_gate_core",
    ]
    
    for core_dir in core_include_dirs:
        if not core_dir.is_dir():
            continue
        for header_file in core_dir.glob("*.hpp"):
            text = read(header_file)
            for pattern in adapter_patterns:
                if re.search(pattern, text, flags=re.IGNORECASE):
                    fail(f"adapter boundary violation in core include: {header_file}")
    
    # Check core source directories for adapter violations
    core_src_dirs = [
        BACKEND / "src/playback_core",
        BACKEND / "src/engine_api",
        BACKEND / "src/session_core",
        BACKEND / "src/decision_ledger_core",
        BACKEND / "src/graph_core",
        BACKEND / "src/network_cache_core",
        BACKEND / "src/demux_core",
        BACKEND / "src/decode_core",
        BACKEND / "src/color_science_core",
        BACKEND / "src/windows_hdr_core",
        BACKEND / "src/video_render_core",
        BACKEND / "src/subtitle_core",
        BACKEND / "src/audio_core",
        BACKEND / "src/playback_inspector_core",
        BACKEND / "src/diagnostic_core",
        BACKEND / "src/resource_core",
        BACKEND / "src/policy_core",
        BACKEND / "src/compliance_core",
        BACKEND / "src/feature_gate_core",
    ]
    
    for core_dir in core_src_dirs:
        if not core_dir.is_dir():
            continue
        for src_file in core_dir.glob("*.cpp"):
            text = read(src_file)
            for pattern in adapter_patterns:
                if re.search(pattern, text, flags=re.IGNORECASE):
                    fail(f"adapter boundary violation in core source: {src_file}")


def check_no_non_entry_files_in_root() -> None:
    # Verify that no non-entry implementation files are added to root
    # Only configuration files, documentation, and project entry points are allowed in root
    
    allowed_root_files = {
        ".gitignore",
        "AGENTS.md",
        "CMakeLists.txt",
        "CMakePresets.json",
        "README.md",
        "vcpkg.json",
        "vcpkg-configuration.json",
    }
    
    allowed_root_extensions = {
        ".md",      # Documentation
        ".txt",     # Text files
        ".json",    # Configuration files
        ".yml",     # YAML configuration
        ".yaml",    # YAML configuration
        ".toml",    # TOML configuration
        ".cfg",     # Configuration files
        ".ini",     # Configuration files
        ".xml",     # XML configuration
        ".cmake",   # CMake files
        ".py",      # Python scripts (for tools)
        ".sh",      # Shell scripts
        ".bat",     # Batch files
        ".ps1",     # PowerShell scripts
    }
    
    for path in ROOT.iterdir():
        if path.is_file():
            # Check if file is in allowed list
            if path.name in allowed_root_files:
                continue
            
            # Check if file has allowed extension
            if path.suffix.lower() in allowed_root_extensions:
                continue
            
            # If not allowed, fail
            fail(f"unexpected root-level implementation file: {path.name}")


# --- P2-001A/B/C Governance Gates ---

P2_DESIGN_DOCS = [
    "docs/p2_real_playback/architecture.md",
    "docs/p2_real_playback/family_tree.md",
    "docs/p2_real_playback/minimal_executable_architecture_slice.md",
    "docs/p2_real_playback/governance_gate_skeleton.md",
    "docs/p2_real_playback/future_contract_placeholders.md",
]

P2_001A_MINIMAL_SLICE_HEADERS = [
    "backend/include/kivo/cinema_engine/source_core/byte_stream_contract.hpp",
    "backend/include/kivo/cinema_engine/source_core/local_file_identity.hpp",
    "backend/include/kivo/cinema_engine/content_integrity_core/content_integrity_contract.hpp",
    "backend/include/kivo/cinema_engine/demux_core/demux_runtime.hpp",
    "backend/include/kivo/cinema_engine/demux_core/kivo_packet.hpp",
    "backend/include/kivo/cinema_engine/decode_core/decode_runtime.hpp",
    "backend/include/kivo/cinema_engine/decode_core/decoded_frame.hpp",
    "backend/include/kivo/cinema_engine/graph_core/graph_node.hpp",
    "backend/include/kivo/cinema_engine/graph_core/graph_edge.hpp",
    "backend/include/kivo/cinema_engine/media_timeline_core/timebase_normalization.hpp",
    "backend/include/kivo/cinema_engine/media_timeline_core/master_clock.hpp",
    "backend/include/kivo/cinema_engine/media_timeline_core/av_sync_policy.hpp",
    "backend/include/kivo/cinema_engine/presentation_timing_core/frame_pacing.hpp",
    "backend/include/kivo/cinema_engine/presentation_timing_core/vsync_policy.hpp",
    "backend/include/kivo/cinema_engine/video_render_core/d3d11_presenter.hpp",
    "backend/include/kivo/cinema_engine/audio_core/wasapi_endpoint.hpp",
    "backend/include/kivo/cinema_engine/resource_core/memory_budget.hpp",
    "backend/include/kivo/cinema_engine/storage_core/source_db.hpp",
    "backend/include/kivo/cinema_engine/storage_core/playback_db.hpp",
    "backend/include/kivo/cinema_engine/storage_core/decision_db.hpp",
]

P2_001C_PLACEHOLDER_HEADERS = [
    "backend/include/kivo/cinema_engine/source_core/source_adapter_contract.hpp",
    "backend/include/kivo/cinema_engine/source_core/source_health_snapshot.hpp",
    "backend/include/kivo/cinema_engine/security_core/redirect_policy.hpp",
    "backend/include/kivo/cinema_engine/network_cache_core/segment_cache_runtime.hpp",
    "backend/include/kivo/cinema_engine/color_science_core/hdr_detection.hpp",
    "backend/include/kivo/cinema_engine/subtitle_core/subtitle_overlay.hpp",
    "backend/include/kivo/cinema_engine/playback_inspector_core/inspector_runtime.hpp",
    "backend/include/kivo/cinema_engine/diagnostic_core/telemetry_ring_buffer.hpp",
    "backend/include/kivo/cinema_engine/governance_core/schema_migration_policy.hpp",
    "backend/include/kivo/cinema_engine/governance_core/claim_vocabulary_gate.hpp",
]


def check_p2_design_docs() -> None:
    for rel in P2_DESIGN_DOCS:
        if not (ROOT / rel).is_file():
            fail(f"missing P2 design doc: {rel}")


def check_p2_001a_minimal_slice_headers() -> None:
    for rel in P2_001A_MINIMAL_SLICE_HEADERS:
        if not (ROOT / rel).is_file():
            fail(f"missing P2-001A minimal slice header: {rel}")


def check_p2_001c_placeholder_headers() -> None:
    for rel in P2_001C_PLACEHOLDER_HEADERS:
        if not (ROOT / rel).is_file():
            fail(f"missing P2-001C placeholder header: {rel}")


def check_p2_header_schema_metadata() -> None:
    all_p2_headers = P2_001A_MINIMAL_SLICE_HEADERS + P2_001C_PLACEHOLDER_HEADERS
    for rel in all_p2_headers:
        text = read(ROOT / rel)
        if "ContractMetadata metadata" not in text:
            fail(f"P2 header missing ContractMetadata metadata field: {rel}")


def check_p2_no_adapter_leak_in_new_headers() -> None:
    adapter_patterns = [
        r"\bWASAPI\b",
        r"\bD3D11\b",
        r"\bDXGI\b",
        r"\bAVFormatContext\b",
        r"\bAVCodecContext\b",
        r"\bAVFrame\b",
        r"\bAVPacket\b",
        r"#\s*include\s*[<\"]windows\.h[>\"]",
    ]
    all_p2_headers = P2_001A_MINIMAL_SLICE_HEADERS + P2_001C_PLACEHOLDER_HEADERS
    for rel in all_p2_headers:
        text = read(ROOT / rel)
        for pattern in adapter_patterns:
            if re.search(pattern, text, flags=re.IGNORECASE):
                fail(f"P2 header contains forbidden adapter type: {rel}")


def main() -> int:
    check_docs()
    check_root_not_flat()
    check_contract_registry()
    check_schema_metadata()
    check_forbidden_includes()
    check_fake_replay_contracts()
    check_gate_sources_exist()
    check_engine_api_boundary_sources()
    check_session_ledger_sources()
    check_fake_engine_api_scenario()
    check_playback_state_machine_sources()
    check_graph_runtime_sources()
    check_media_pipeline_contract_sources()
    check_color_hdr_presenter_sources()
    check_audio_subtitle_sources()
    check_inspector_diagnostic_sources()
    check_policy_compliance_resource_sources()
    check_adapter_boundary_sources()
    check_adapter_boundary_directory_rules()
    check_no_non_entry_files_in_root()
    check_redaction_gate_model()
    check_invariant_gate_model()
    # P2-001A/B/C gates
    check_p2_design_docs()
    check_p2_001a_minimal_slice_headers()
    check_p2_001c_placeholder_headers()
    check_p2_header_schema_metadata()
    check_p2_no_adapter_leak_in_new_headers()
    print("PASS: P0/P1/P2 governance gates")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
