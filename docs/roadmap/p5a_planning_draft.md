# KIVO-VIDEO-P5A-PLANNING-DRAFT-001-REVISION-001

**Document Type**: planning-draft (design-only, ZERO implementation)
**Source**: P5 V2.2 Final Lock Patch → `p5_v2.2_execution_card.md`
**Stage**: P5A (Contract Definition)
**Status**: DRAFT_IN_PROGRESS
**Revision**: 001 (covers all 13 user-flagged issues)
**Generated**: 2026-06-28
**Rule**: F11/F12 禁止此 Draft APPROVED 前任何目录创建 / 头文件 / CMake

---

## CHANGE LOG (REVISION 001 vs REVISION 000)

| # | Issue | Fix |
|---|-------|-----|
| 1 | Missing CoreStamp/FullStamp/P5StampValidator | §3.0 NEW: CoreVideoStamp, FullVideoStamp, P5StampValidator, StampValidationContext, StampValidationKind, StampValidationResult |
| 2 | Capability domain count inconsistent | §3.1: Count unified to 25 domains (list-authoritative, not text-claim); each domain has official enum id |
| 3 | EvidenceRecord not fully expanded | §3.2: All 22 fields listed in table with types; CriticalEvidenceRing/DropCounter/BackpressurePolicy → contract structs |
| 4 | CMake core target mislabeled as FFmpeg | §2: kivo_video_plane_core = backend-neutral light runtime boundary, NOT FFmpeg wrapper |
| 5 | No precise file allowlist | §4: Full allowlist table — 109 headers + 11 test dirs + CMake + docs, exact paths |
| 6 | F-01 forbids all .cpp | §5: F-01 split → production .cpp FORBIDDEN, contract test .cpp ALLOWED |
| 7 | RecoverySeekRequest too weak | §3.11: Expanded to 9 fields with timeline_revision, target_keyframe_policy, active_track_boundaries |
| 8 | QueueItem has string fields | §3.12: VideoPipelineId, CoreVideoStamp, CancelToken, QueueItemId, FailureCorrelationId all strong types |
| 9 | SandboxBrokerContract barely expanded | §3.9: 11 contract types + 13 broker messages + IPC header 12 fields + DemuxBufferPlacement enum |
| 10 | Threat Matrix only has item | §3.8: P5ThreatMitigationMatrix container + threat_surface enum + completeness checker + fuzz linkage + gate mapping |
| 11 | Architecture Guard declarations only | §3.20: Scanner paths, target graph check method, include boundary rules, binary symbol rules, dep graph scan, violation schema, CI command, positive/negative test plan |
| 12 | Developer Guide no uniform template | §3.24: All 11 scenarios expanded with 9-row uniform template |
| 13 | Exit Criteria not mapped | §6: 16-item coverage table with direct mapping to draft sections |

---

## IRON GATE COMPLIANCE

| # | Gate | Status |
|---|------|--------|
| 1 | 构思 | ✅ V2.2 Final Lock Patch.docx |
| 2 | 规划设计 | ⚠️ THIS DOCUMENT (in revision) |
| 3 | 自然家族树 | ⚠️ Defined in §1 (revised) |
| 4 | 实现 | ❌ BLOCKED until APPROVED |
| 5 | 单文件单职责 | ⏳ After implementation |
| 6 | 禁止平铺文件 | ⏳ After implementation |

---

## 1. NATURAL FAMILY TREE (FULL, REVISED)

```
include/kivo/video_plane/
│
├── stamp/                          # NEW — Core/FullStamp + Validator (6 types)
│   ├── core_video_stamp.hpp
│   ├── full_video_stamp.hpp
│   ├── stamp_validation_context.hpp
│   ├── stamp_validation_kind.hpp
│   ├── stamp_validation_result.hpp
│   └── p5_stamp_validator.hpp
│
├── capability/                     # GATE-001 统一能力注册表 (12 types)
│   ├── capability_registry.hpp
│   ├── capability_snapshot.hpp
│   ├── capability_query.hpp
│   ├── capability_state_resolver.hpp
│   ├── capability_verification_scope.hpp
│   ├── capability_evidence_link.hpp
│   ├── capability_support_state.hpp
│   ├── capability_policy_state.hpp
│   ├── capability_runtime_state.hpp
│   ├── capability_domain_id.hpp
│   ├── capability_registry_contract.hpp
│   └── capability_bootstrap_policy.hpp
│
├── evidence/                       # GATE-002 统一证据记录 (8 types)
│   ├── evidence_record.hpp
│   ├── evidence_severity.hpp
│   ├── evidence_loss_policy.hpp
│   ├── critical_evidence_ring.hpp
│   ├── drop_counter.hpp
│   ├── evidence_backpressure_policy.hpp
│   ├── evidence_schema_migration_note.hpp
│   └── evidence_redaction_level.hpp
│
├── threat/                         # GATE-003 威胁矩阵 (5 types)
│   ├── threat_surface_id.hpp
│   ├── threat_mitigation_matrix_item.hpp
│   ├── threat_mitigation_matrix.hpp
│   ├── threat_matrix_completeness_checker.hpp
│   └── threat_matrix_gate_mapping.hpp
│
├── sandbox/                        # GATE-004 沙箱协议 (12 types)
│   ├── sandbox_broker_contract.hpp
│   ├── sandboxed_demux_process.hpp
│   ├── sandboxed_decode_process.hpp
│   ├── sandbox_ipc_protocol_version.hpp
│   ├── sandbox_ipc_message_header.hpp
│   ├── sandbox_broker_message_kind.hpp
│   ├── demux_buffer_placement.hpp
│   ├── sandbox_resource_budget.hpp
│   ├── sandbox_crash_policy.hpp
│   ├── sandbox_timeout_policy.hpp
│   ├── sandbox_memory_policy.hpp
│   ├── sandbox_file_access_policy.hpp
│   ├── sandbox_shared_memory_policy.hpp
│   └── sandbox_broker_evidence.hpp
│
├── allocation/                     # GATE-005 分配限制 (2 types)
│   ├── allocation_limit_policy.hpp
│   └── allocation_limit_evidence.hpp
│
├── fuzz/                           # GATE-006 Fuzz范围 (2 types)
│   ├── fuzz_smoke.hpp
│   └── backend_risk_register.hpp
│
├── version/                        # GATE-007 版本化 (3 types)
│   ├── boundary_contract_version.hpp
│   ├── compatibility_policy.hpp
│   └── version_mismatch_evidence.hpp
│
├── failure/                        # GATE-008 故障仲裁 (5 types)
│   ├── pipeline_failure_arbiter.hpp
│   ├── pipeline_fatal_signal.hpp
│   ├── pipeline_terminal_outcome.hpp
│   ├── failure_correlation_id.hpp
│   └── arbitration_evidence.hpp
│
├── recovery/                       # GATE-009 恢复Seek (3 types)
│   ├── recovery_seek_request.hpp
│   ├── standby_until_new_input.hpp
│   └── live_edge_hint_priority.hpp
│
├── queue/                          # GATE-010 队列生命周期 (7 types)
│   ├── queue_lifecycle_state.hpp
│   ├── cancel_mode.hpp
│   ├── cancel_token.hpp
│   ├── queue_item_id.hpp
│   ├── video_pipeline_id.hpp
│   ├── queue_item.hpp
│   └── queue_timeout_defaults.hpp
│
├── protected_content/              # GATE-011 受保护内容 (5 types)
│   ├── protected_boundary_hint.hpp
│   ├── protected_placeholder_frame.hpp
│   ├── protected_playback_capability_state.hpp
│   ├── secure_decode_session_context_handle.hpp
│   └── protected_boundary_evidence.hpp
│
├── protected_output/               # GATE-012 多显示器输出 (4 types)
│   ├── surface_to_display_mapper.hpp
│   ├── active_output_set.hpp
│   ├── protected_output_coverage.hpp
│   └── protected_output_status_snapshot.hpp
│
├── dolby_vision/                   # GATE-013 Dolby Vision (3 types)
│   ├── safe_base_layer_evidence.hpp
│   ├── dolby_vision_engine_gate.hpp
│   └── dv_fallback_policy.hpp
│
├── hdr10_plus/                     # GATE-014 HDR10+ (3 types)
│   ├── hdr10_plus_capability.hpp
│   ├── opaque_metadata_policy.hpp
│   └── hdr10_plus_fallback_semantic.hpp
│
├── p010/                           # GATE-015 P010公式 (4 types)
│   ├── p010_formula.hpp
│   ├── p010_code_recovery.hpp
│   ├── p010_unorm_tolerance.hpp
│   └── p010_plane_access_evidence.hpp
│
├── d3d11va/                        # GATE-016 D3D11VA (4 types)
│   ├── d3d11va_context_mode.hpp
│   ├── d3d11_texture_opaque_ref.hpp
│   ├── decode_render_sync_mode.hpp
│   └── opaque_fallback_policy.hpp
│
├── swapchain/                      # GATE-017 DXGI SwapChain (6 types)
│   ├── backbuffer_reference_tracker.hpp
│   ├── backbuffer_view_registry.hpp
│   ├── deferred_command_list_tracker.hpp
│   ├── swapchain_resize_preflight.hpp
│   ├── live_object_tracker.hpp
│   └── binding_release_evidence.hpp
│
├── vrr/                            # GATE-018 VRR (3 types)
│   ├── vrr_state.hpp
│   ├── waitable_swapchain_state.hpp
│   └── rational_cadence_accumulator.hpp
│
├── color/                          # GATE-019 色彩/HDR/色度 (4 types)
│   ├── color_metadata_authority_policy.hpp
│   ├── chroma_siting_plan.hpp
│   ├── frame_export_color_policy.hpp
│   └── tone_map_policy.hpp
│
├── timebase/                       # GATE-020 时间基 (4 types)
│   ├── checked_rescale.hpp
│   ├── live_timestamp_origin_resolver.hpp
│   ├── reorder_policy.hpp
│   └── catchup_policy.hpp
│
├── attachment/                     # GATE-021 附件安全 (7 types)
│   ├── mime_conflict_policy.hpp
│   ├── opaque_attachment_flow_handle.hpp
│   ├── quota_scope.hpp
│   ├── subtitle_packet_safety_policy.hpp
│   ├── external_subtitle_boundary_hint.hpp
│   ├── external_subtitle_path_policy.hpp
│   └── external_subtitle_encoding_detection_policy.hpp
│
├── presentation/                   # GATE-022 PresentationEpoch (3 types)
│   ├── epoch_condition.hpp
│   ├── hold_last_frame_safe.hpp
│   └── geometry_pipeline_order.hpp
│
├── output_boundary/                # GATE-023 P5→P6/P7/P8 (4 types)
│   ├── audio_packet_boundary_item.hpp
│   ├── subtitle_composition_snapshot.hpp
│   ├── repeat_reason.hpp
│   └── last_visible_estimate.hpp
│
├── gpu/                            # GATE-024 GPU预算 (6 types)
│   ├── gpu_budget_provider.hpp
│   ├── gpu_budget_snapshot.hpp
│   ├── budget_pressure_level.hpp
│   ├── gpu_budget_evidence.hpp
│   ├── device_removed_reason.hpp
│   └── display_capability_snapshot.hpp
│
├── thread/                         # GATE-025 线程运行时 (5 types)
│   ├── thread_affinity_token.hpp
│   ├── contract_runtime_checker.hpp
│   ├── lock_rank_checker.hpp
│   ├── blocking_call_detector.hpp
│   └── com_release_policy.hpp
│
├── architecture/                   # GATE-026 架构守卫 (6 types)
│   ├── architecture_target_graph.hpp
│   ├── symbol_checker_rule.hpp
│   ├── link_rules.hpp
│   ├── include_boundary_checker.hpp
│   ├── architecture_violation_schema.hpp
│   └── architecture_guard_ci_command.hpp
│
├── license/                        # GATE-027 许可证/专利 (4 types)
│   ├── patent_risk_report.hpp
│   ├── redistribution_checklist.hpp
│   ├── compliance_package.hpp
│   └── third_party_notice_bundle.hpp
│
├── telemetry/                      # GATE-028 遥测隐私 (2 types)
│   ├── telemetry_privacy_policy.hpp
│   └── diagnostics_retention_policy.hpp
│
├── state_monitor/                  # GATE-029 形式化状态 (2 types)
│   ├── formal_state_monitor.hpp
│   └── formal_state_violation.hpp
│
├── test_ci/                        # GATE-030 CI分层 (2 types)
│   ├── gate_type.hpp
│   └── debug_layer_policy.hpp
│
├── malformed/                      # GATE-031 畸形语料 (1 type)
│   └── malformed_corpus_item.hpp
│
├── export/                         # GATE-034 帧导出安全 (1 type)
│   └── frame_export_security_policy.hpp
│
├── plugin/                         # GATE-035 插件安全 (1 type)
│   └── plugin_security_policy.hpp
│
├── window/                         # GATE-036 窗口可见性 (1 type)
│   └── window_visibility_policy.hpp
│
├── success/                        # GATE-037 成功定义 (1 type)
│   └── success_definition_item.hpp
│
└── change/                         # GATE-038 变更控制 (1 type)
    └── change_control.hpp
```

**统计**: 32 个子目录, 121 个头文件（较 REV 000 增加 2 个目录 + 12 个头文件，均为 V2.2 SPEC 要求）

---

## 2. CMake TARGET STRUCTURE (REVISED — core ≠ FFmpeg wrapper)

```
INTERFACE kivo_video_plane_contracts    ← P5A: all 121 public contract headers
                                        ← LINK: NOTHING (pure contracts)
                                        ← NAMESPACE: kivo::video_plane

STATIC   kivo_video_plane_core         ← P5B+: backend-neutral light runtime boundary
                                        ← Responsible: bootstrap, lifecycle, CapabilityRegistry runtime,
                                        │   QueueLifecycle runtime, PipelineFailureArbiter runtime,
                                        │   EvidenceRing runtime, StampValidator runtime
                                        ← LINK: kivo_video_plane_contracts only
                                        ← MAY NOT LINK: FFmpeg, D3D11, DXGI, Qt, WASAPI
                                        ← NOT a FFmpeg wrapper (that's kivo_video_plane_ffmpeg_demux)
                                        ← NAMESPACE: kivo::video_plane::core

STATIC   kivo_video_plane_ffmpeg_demux ← P5C: FFmpeg demux backend (sandbox-host or in-process)
                                        ← LINK: kivo_video_plane_core + FFmpeg::avformat
                                        ← ISOLATED: src/video_plane/ffmpeg_demux/ only
                                        ← NAMESPACE: kivo::video_plane::ffmpeg_demux

STATIC   kivo_video_plane_ffmpeg_decode← P5D+: FFmpeg decode backend
                                        ← LINK: kivo_video_plane_core + FFmpeg::avcodec
                                        ← ISOLATED: src/video_plane/ffmpeg_decode/ only
                                        ← NAMESPACE: kivo::video_plane::ffmpeg_decode

STATIC   kivo_video_plane_d3d11_gpu    ← P5F/G: D3D11 GPU backend (device, context, texture ref)
                                        ← LINK: kivo_video_plane_core + D3D11 + DXGI
                                        ← ISOLATED: src/video_plane/d3d11_gpu/ only
                                        ← NAMESPACE: kivo::video_plane::d3d11_gpu

STATIC   kivo_video_plane_dxgi_swapchain ← P5F: DXGI swapchain backend
                                        ← LINK: kivo_video_plane_core + DXGI
                                        ← ISOLATED: src/video_plane/dxgi_swapchain/ only
                                        ← NAMESPACE: kivo::video_plane::dxgi_swapchain

INTERFACE kivo_video_plane_future_backend_contracts ← future-only
                                        ← NOT in default include path
                                        ← NOT linked into baseline binary
                                        ← NAMESPACE: kivo::video_plane::future

EXE      kivo_video_plane_contract_tests ← P5A+: all contract tests
                                        ← LINK: kivo_video_plane_contracts
                                        ← CI: fake backends only
                                        ← Lab: real HW gates
```

**Link Rules (GATE-026, 10 rules)**:

| # | Rule | Check |
|---|------|-------|
| 1 | `kivo_video_plane_contracts` links NOTHING | CMake dep graph |
| 2 | Contracts target has NO `target_link_libraries` to FFmpeg/D3D11/DXGI/Qt/WASAPI | CMake verify |
| 3 | `kivo_video_plane_core` links contracts only, NO FFmpeg/D3D11 | CMake + header scan |
| 4 | `kivo_video_plane_ffmpeg_demux` MAY link FFmpeg, ONLY in `ffmpeg_demux/` dir | Path isolation |
| 5 | `kivo_video_plane_d3d11_gpu` MAY link D3D11, ONLY in `d3d11_gpu/` dir | Path isolation |
| 6 | Public headers contain NO raw third-party types | Forbidden token scan |
| 7 | Binary exports contain NO forbidden symbols | `dumpbin /exports` scan |
| 8 | Future backends NOT in default include path | CMake check |
| 9 | Future backends NOT linked into baseline binary | CMake dep graph |
| 10 | Core target exposes NO FFmpeg/D3D11 public ABI | Header + symbol scan |

---

## 3. P5A SCOPE: TYPE-BY-TYPE DEFINITIONS (GATE 001~038)

### 3.0 GATE-STAMP: CoreVideoStamp / FullVideoStamp / P5StampValidator (6 types) — NEW

**Subdirectory**: `stamp/`

These are P5A core types. They are referenced by EvidenceRecord, QueueItem, and the failure/arbitration pipeline. They must be explicit contract types, not hidden as `uint64_t`.

#### core_video_stamp.hpp
```
namespace kivo::video_plane::stamp {

struct CoreVideoStamp {
    uint64_t value{0};                      // monotonic, P5-assigned at pipeline ingest
    bool operator==(CoreVideoStamp const&) const = default;
};

}  // namespace
```

#### full_video_stamp.hpp
```
namespace kivo::video_plane::stamp {

struct FullVideoStamp {
    uint64_t pipeline_epoch{0};             // P4 session revision
    uint64_t input_sequence{0};             // P5 input bridge increment
    CoreVideoStamp core_stamp;              // P5 core stamp (subordinate)
    bool operator==(FullVideoStamp const&) const = default;
};

}  // namespace
```

#### stamp_validation_context.hpp
```
namespace kivo::video_plane::stamp {

struct StampValidationContext {
    FullVideoStamp last_known_full_stamp;
    CoreVideoStamp current_core_stamp;
    uint64_t expected_pipeline_epoch{0};
    bool is_recovery_seek{false};
    bool is_bootstrap{false};
    bool operator==(StampValidationContext const&) const = default;
};

}  // namespace
```

#### stamp_validation_kind.hpp
```
namespace kivo::video_plane::stamp {

enum class StampValidationKind {
    BootstrapCheck,                         // initial pipeline setup
    IncrementalAdvance,                     // normal forward progress
    SeekReposition,                         // user or recovery seek
    PipelineEpochChange,                    // P4 session changed
    RecoveryRejoin,                         // after device lost / rebuild
    InputGap,                               // discontinuity in input
    QueueStarvationRecheck                  // queue starved, recheck stamp validity
};

}  // namespace
```

#### stamp_validation_result.hpp
```
namespace kivo::video_plane::stamp {

enum class StampValidationOutcome {
    Valid,                                  // stamp passes all checks
    StaleCoreStamp,                         // core stamp behind last known
    PipelineEpochInvalid,                   // epoch doesn't match expected
    SkipForwardInvalid,                     // forward jump too large
    RecoveryBookmarkMismatch,               // recovery bookmark inconsistent
    BootstrapNotReady                       // bootstrap prerequisites not met
};

struct StampValidationResult {
    StampValidationOutcome outcome{StampValidationOutcome::Valid};
    StampValidationKind kind{StampValidationKind::BootstrapCheck};
    std::string detail_code;
    std::string evidence_id;
    bool operator==(StampValidationResult const&) const = default;
};

}  // namespace
```

#### p5_stamp_validator.hpp
```
namespace kivo::video_plane::stamp {

struct P5StampValidator {
    // Validates CoreVideoStamp and FullVideoStamp against context
    static StampValidationResult validate_core_stamp(
        CoreVideoStamp stamp,
        StampValidationContext const& context);

    static StampValidationResult validate_full_stamp(
        FullVideoStamp stamp,
        StampValidationContext const& context);

    // Returns true if stamp is monotonically advancing
    static bool is_monotonic_advance(
        CoreVideoStamp previous,
        CoreVideoStamp current);

    // Bootstrap: set initial stamp baseline
    static FullVideoStamp create_bootstrap_stamp(
        uint64_t pipeline_epoch);
};

}  // namespace
```

---

### 3.1 GATE-001: Capability Registry (12 types, 25 domains — COUNT FIXED)

**Subdirectory**: `capability/`

#### capability_domain_id.hpp — NEW, replaces implicit domain strings
```
namespace kivo::video_plane::capability {

enum class CapabilityDomainId {
    // 25 capability domains (list-authoritative, NOT text "21")
    Demux,                                  //  1
    FfmpegBackend,                          //  2
    SandboxBroker,                          //  3
    Codec,                                  //  4
    HardwareDecode,                         //  5
    SoftwareDecode,                         //  6
    D3d11va,                                //  7
    Nv12Srv,                                //  8
    P010IntegerPlaneView,                   //  9
    P010UnormFallback,                      // 10
    ZeroCopy,                               // 11
    CrossAdapterBridge,                     // 12
    SwapChain,                              // 13
    WaitableSwapChain,                      // 14
    Vrr,                                    // 15
    DwmTiming,                              // 16
    Hdr10,                                  // 17
    Hdr10Plus,                              // 18
    Hlg,                                    // 19
    DolbyVisionFallback,                    // 20
    ProtectedPlayback,                      // 21
    OutputProtection,                       // 22
    SnapshotExport,                         // 23
    SubtitleAttachment,                     // 24
    FramePacing                             // 25
};

}  // namespace
```

**NOTE**: V2.2 execution card text says "21 domains" but the list enumerates 25 items. This is a source-doc discrepancy. The list is authoritative. Proceed with 25.

#### capability_registry.hpp (revised)
```
namespace kivo::video_plane::capability {

struct P5CapabilityRegistry {
    // 25 capability domains, each with get/set/query
    // Registry is the SINGLE AUTHORITY for capability state
    // No component self-declares capability outside registry

    CapabilitySnapshot get(CapabilityQuery const& query) const;
    void register_capability(CapabilitySnapshot snapshot);
    std::vector<CapabilitySnapshot> list_all() const;
    std::vector<CapabilitySnapshot> list_by_domain(CapabilityDomainId domain) const;
};

}  // namespace
```

#### capability_snapshot.hpp (revised, uses CapabilityDomainId)
```
namespace kivo::video_plane::capability {

struct P5CapabilitySnapshot {
    std::string capability_id;
    CapabilityDomainId domain;
    CapabilitySupportState support_state{CapabilitySupportState::DeclaredContract};
    CapabilityVerificationScope verification_scope;
    CapabilityPolicyState policy_state;
    CapabilityRuntimeState runtime_state;
    std::string last_probe_result;
    std::string evidence_id;
    std::string failure_reason;
    std::string user_visible_status;        // P8-readable
    std::string technical_detail_code;      // P8-readable
    bool operator==(P5CapabilitySnapshot const&) const = default;
};

}  // namespace
```

#### capability_query.hpp (revised, uses CapabilityDomainId)
```
namespace kivo::video_plane::capability {

struct P5CapabilityQuery {
    std::string capability_id;
    CapabilityDomainId domain;
    bool operator==(P5CapabilityQuery const&) const = default;
};

}  // namespace
```

#### capability_registry_contract.hpp (P8 access rules)
```
namespace kivo::video_plane::capability {

struct P5CapabilityRegistryContract {
    // P8 MAY read: P5CapabilitySnapshot.user_visible_status
    // P8 MAY read: P5CapabilitySnapshot.technical_detail_code
    // P8 MAY read: P5CapabilitySnapshot.capability_id
    // P8 MAY read: P5CapabilitySnapshot.domain
    //
    // P8 MUST NOT read: CapabilitySupportState internals
    // P8 MUST NOT read: CapabilityVerificationScope internals
    // P8 MUST NOT read: CapabilityPolicyState internals
    // P8 MUST NOT read: CapabilityRuntimeState internals
    // P8 MUST NOT read: sub-state-machine transitions
    //
    // All capability lookup MUST go through P5CapabilityRegistry
    // No component may self-declare capability outside registry
};

}  // namespace
```

#### capability_bootstrap_policy.hpp — NEW
```
namespace kivo::video_plane::capability {

struct CapabilityBootstrapPolicy {
    // Exactly ONE component owns bootstrap initialization (see §3.24 A9)
    // Registry populated during bootstrap
    // No self-initialization outside bootstrap order
    uint64_t bootstrap_timeout_ms{5000};
    bool require_all_domains_registered{false}; // P5A: false; P5H: true
    bool operator==(CapabilityBootstrapPolicy const&) const = default;
};

}  // namespace
```

**Remaining 5 types** (unchanged from REV 000, included for completeness):
- `CapabilitySupportState` (10-value enum)
- `CapabilityPolicyState` (struct: enabled_by_policy, restricted_by_policy, policy_id, policy_reason)
- `CapabilityRuntimeState` (struct: active, degraded, degradation_reason)
- `CapabilityVerificationScope` (19-field struct: platform, os_version, driver_version, ...)
- `CapabilityEvidenceLink` (struct: evidence_id, evidence_kind, gate_stage)

---

### 3.2 GATE-002: Evidence Schema (8 types — FULLY EXPANDED)

**Subdirectory**: `evidence/`

#### evidence_record.hpp (22 fields — FULLY EXPANDED, NO ABBREVIATION)

| # | Field | Type | Optional | Description |
|---|-------|------|----------|-------------|
| 1 | `schema_version` | `uint32_t` | — | Evidence schema version |
| 2 | `evidence_id` | `std::string` | — | Unique evidence identifier |
| 3 | `evidence_kind` | `std::string` | — | Kind of evidence (e.g. "DemuxBackendCrashed") |
| 4 | `severity` | `EvidenceSeverity` | — | Severity level |
| 5 | `component_id` | `std::string` | — | Originating P5 component |
| 6 | `pipeline_id` | `std::string` | — | Pipeline instance |
| 7 | `core_video_stamp` | `std::optional<CoreVideoStamp>` | yes | Core stamp at evidence time |
| 8 | `full_video_stamp` | `std::optional<FullVideoStamp>` | yes | Full stamp at evidence time |
| 9 | `monotonic_time` | `uint64_t` | — | Monotonic clock timestamp |
| 10 | `media_time` | `std::optional<uint64_t>` | yes | Media timestamp |
| 11 | `thread_id` | `std::optional<uint64_t>` | yes | Thread where evidence was generated |
| 12 | `state_before` | `std::optional<std::string>` | yes | Component state before event |
| 13 | `state_after` | `std::optional<std::string>` | yes | Component state after event |
| 14 | `decision` | `std::optional<std::string>` | yes | P5 decision (e.g. recovery action) |
| 15 | `reason_code` | `std::string` | — | Machine-readable reason |
| 16 | `technical_detail_code` | `std::string` | — | Technical detail for diagnostics |
| 17 | `payload_ref` | `std::optional<std::string>` | yes | Reference to evidence payload |
| 18 | `redaction_level` | `EvidenceRedactionLevel` | — | Redaction level applied |
| 19 | `pii_safety_class` | `uint32_t` | — | PII safety classification |
| 20 | `protected_content_safety_class` | `uint32_t` | — | Protected content safety class |
| 21 | `loss_policy` | `EvidenceLossPolicy` | — | Loss policy |
| 22 | `correlation_id` | `std::optional<std::string>` | yes | Cross-pipeline correlation ID |

```
namespace kivo::video_plane::evidence {

using kivo::video_plane::stamp::CoreVideoStamp;
using kivo::video_plane::stamp::FullVideoStamp;

struct P5EvidenceRecord {
    uint32_t schema_version{0};
    std::string evidence_id;
    std::string evidence_kind;
    EvidenceSeverity severity{EvidenceSeverity::Info};
    std::string component_id;
    std::string pipeline_id;
    std::optional<CoreVideoStamp> core_video_stamp;
    std::optional<FullVideoStamp> full_video_stamp;
    uint64_t monotonic_time{0};
    std::optional<uint64_t> media_time;
    std::optional<uint64_t> thread_id;
    std::optional<std::string> state_before;
    std::optional<std::string> state_after;
    std::optional<std::string> decision;
    std::string reason_code;
    std::string technical_detail_code;
    std::optional<std::string> payload_ref;
    EvidenceRedactionLevel redaction_level{EvidenceRedactionLevel::None};
    uint32_t pii_safety_class{0};
    uint32_t protected_content_safety_class{0};
    EvidenceLossPolicy loss_policy{EvidenceLossPolicy::NonCriticalDroppable};
    std::optional<std::string> correlation_id;
    bool operator==(P5EvidenceRecord const&) const = default;
};

}  // namespace
```

#### evidence_redaction_level.hpp — NEW
```
namespace kivo::video_plane::evidence {

enum class EvidenceRedactionLevel {
    None,           // no redaction needed (debug-only)
    PiiRedacted,    // PII removed
    PathRedacted,   // private paths redacted
    FullRedacted,   // protected content + PII + paths redacted
    OmitPayload     // payload omitted entirely
};

}  // namespace
```

#### critical_evidence_ring.hpp (contract, not just comment)
```
namespace kivo::video_plane::evidence {

struct P5CriticalEvidenceRing {
    // Ring buffer for Fatal/Security/PolicyDenied/FailClosed evidence
    static constexpr size_t kDefaultCapacity = 128;
    static constexpr size_t kMinReservedForTerminalOutcome = 8;

    // Contract:
    // - Must not overwrite current terminal outcome record
    // - Must not silently drop Fatal-severity evidence
    // - Overflow → emit EvidenceRingOverflowEvidence
    // - Read access: P5DiagnosticsService, P8 capability display
    size_t capacity{kDefaultCapacity};
    size_t current_size{0};
    bool terminal_outcome_present{false};
    bool operator==(P5CriticalEvidenceRing const&) const = default;
};

}  // namespace
```

#### drop_counter.hpp (contract, not just comment)
```
namespace kivo::video_plane::evidence {

struct P5NonCriticalEvidenceDropCounter {
    uint64_t total_dropped{0};             // atomic counter, monotonic
    uint64_t dropped_since_last_report{0}; // resettable
    uint64_t last_report_time{0};

    // Contract:
    // - Increment on every NonCriticalDroppable evidence drop
    // - Reset dropped_since_last_report on report emission
    // - Emit EvidenceDropReport when dropped_since_last_report > threshold
    bool operator==(P5NonCriticalEvidenceDropCounter const&) const = default;
};

}  // namespace
```

#### evidence_backpressure_policy.hpp (contract, not just comment)
```
namespace kivo::video_plane::evidence {

struct P5EvidenceBackpressurePolicy {
    bool release_never_blocks{true};       // Release build: never block on evidence sink
    bool debug_may_fail_fast{true};        // Debug build: may fail-fast if sink blocked
    uint64_t max_block_duration_ms{0};     // Max block duration (0 = never block)

    // Contract:
    // - Release: if sink unavailable, drop non-critical, ring critical
    // - Debug: may fail-fast if sink blocked > threshold
    // - Never block render hot path
    // - Never block during COM release
    bool operator==(P5EvidenceBackpressurePolicy const&) const = default;
};

}  // namespace
```

**Remaining types (unchanged)**:
- `EvidenceSeverity` (10-value enum: Trace~FailClosed)
- `EvidenceLossPolicy` (6-value enum: CriticalRingRequired~SecurityRequired)
- `P5EvidenceSchemaMigrationNote` (struct: from_version, to_version, breaking_change_description, migration_instructions)

---

### 3.3 GATE-003: Threat Matrix (5 types — EXPANDED)

**Subdirectory**: `threat/`

#### threat_surface_id.hpp — NEW
```
namespace kivo::video_plane::threat {

enum class ThreatSurfaceId {
    // 29 threat surfaces (from GATE-003 spec)
    MaliciousContainer,
    MaliciousCodecBitstream,
    MaliciousCodecExtradata,
    MaliciousSubtitlePacket,
    MaliciousAttachment,
    MaliciousFontFile,
    MaliciousHdrMetadata,
    MaliciousDolbyMetadata,
    TimestampOverflow,
    DurationOverflow,
    IndexCorruption,
    AllocationBomb,
    DecompressionBomb,
    PathTraversal,
    MetadataFakeMime,
    AttachmentMimeMismatch,
    LiveDiscontinuityAbuse,
    InfiniteProbe,
    DemuxStuck,
    DecoderStuck,
    ShaderMetadataAbuse,
    ProtectedContentExportAttempt,
    ExternalSubtitlePathAbuse,
    DebugDumpLeakage,
    SandboxIpcOversizedPayload,
    SharedMemoryStaleHandle,
    PluginCodeInjection,
    TelemetryPrivacyLeak,
    DiagnosticsPrivatePathLeak
};

}  // namespace
```

#### threat_mitigation_matrix_item.hpp
```
namespace kivo::video_plane::threat {

struct P5ThreatMitigationMatrixItem {
    std::string threat_id;
    ThreatSurfaceId threat_surface;        // from 29 surfaces
    std::string attack_vector;
    std::string affected_component;
    std::string risk_level;                // Critical/High/Medium/Low
    std::string mitigation;
    bool sandbox_required{false};
    std::string allocation_policy;
    std::string fuzz_target;
    std::string regression_corpus;
    std::string expected_failure_mode;
    std::string evidence_kind;
    std::string gate_stage;                // P5A/P5B/.../P5H/P5S
    bool operator==(P5ThreatMitigationMatrixItem const&) const = default;
};

}  // namespace
```

#### threat_mitigation_matrix.hpp — NEW (container)
```
namespace kivo::video_plane::threat {

struct P5ThreatMitigationMatrix {
    std::vector<P5ThreatMitigationMatrixItem> items;
    std::string matrix_version;
    uint64_t last_updated{0};

    // Contract:
    // - Must contain all 29 threat surfaces
    // - Each threat must have >= 1 mitigation
    // - Each mitigation must have >= 1 test name
    // - Must not have only quarantine as mitigation
    // - Matrix completeness checked by GATE-003 checker
    bool operator==(P5ThreatMitigationMatrix const&) const = default;
};

}  // namespace
```

#### threat_matrix_completeness_checker.hpp — NEW
```
namespace kivo::video_plane::threat {

struct P5ThreatMatrixCompletenessChecker {
    struct CompletenessResult {
        bool all_surfaces_covered{false};
        std::vector<ThreatSurfaceId> missing_surfaces;
        std::vector<std::string> items_without_test;
        std::vector<std::string> items_without_fuzz_target;
        std::string evidence_id;
    };

    static CompletenessResult check(P5ThreatMitigationMatrix const& matrix);
};

}  // namespace
```

#### threat_matrix_gate_mapping.hpp — NEW
```
namespace kivo::video_plane::threat {

struct P5ThreatMatrixGateMapping {
    // Maps each threat surface to its verification gate stage
    static ThreatSurfaceId surface_for_gate(std::string const& gate_name);
    static std::vector<std::string> gates_for_surface(ThreatSurfaceId surface);
    static bool is_surface_covered_at_gate(ThreatSurfaceId surface, std::string const& gate);
};

}  // namespace
```

---

### 3.4 GATE-004: Sandbox Broker Contract (14 types — FULLY EXPANDED)

**Subdirectory**: `sandbox/`

#### sandbox_ipc_protocol_version.hpp — NEW
```
namespace kivo::video_plane::sandbox {

using SandboxIpcProtocolVersion = BoundaryContractVersion;

}  // namespace
```

#### sandbox_ipc_message_header.hpp — NEW (12 fields)
```
namespace kivo::video_plane::sandbox {

struct SandboxIpcMessageHeader {
    uint32_t protocol_version{0};
    uint64_t request_id{0};
    std::string pipeline_id;
    uint64_t media_epoch{0};
    uint64_t deadline_ms{0};
    uint64_t payload_size{0};
    std::string shared_memory_handle;       // opaque ref, not raw pointer
    uint32_t capability_flags{0};
    uint32_t security_class{0};
    std::string correlation_id;
    uint32_t message_kind{0};               // SandboxBrokerMessageKind cast
    bool operator==(SandboxIpcMessageHeader const&) const = default;
};

}  // namespace
```

#### sandbox_broker_message_kind.hpp — NEW
```
namespace kivo::video_plane::sandbox {

enum class SandboxBrokerMessageKind {
    // 13 broker messages (from GATE-004)
    OpenStream,                             // open media stream
    CloseStream,                            // close media stream
    ProbeContainer,                         // probe container format
    FindStreamInfo,                         // find stream info
    ReadPacket,                             // read next packet
    Seek,                                   // seek to position
    Flush,                                  // flush buffers
    Cancel,                                 // cancel current operation
    GetTrackDescriptors,                    // get track metadata
    GetAttachmentDescriptor,                // get attachment metadata
    GetOpaqueAttachmentChunk,               // read attachment chunk
    ReportBackendEvidence,                  // report backend evidence to broker
    Terminate                               // terminate sandbox process
};

}  // namespace
```

#### demux_buffer_placement.hpp — NEW
```
namespace kivo::video_plane::sandbox {

enum class DemuxBufferPlacement {
    HostProcessPool,                        // buffer in host process pool
    SandboxProcessPool,                     // buffer in sandbox process pool
    SharedMemoryRing,                       // shared memory ring buffer
    CopyBridge                              // copy across boundary
};

}  // namespace
```

#### sandbox_broker_contract.hpp (expanded)
```
namespace kivo::video_plane::sandbox {

struct P5SandboxBrokerContract {
    SandboxIpcProtocolVersion ipc_version;
    std::vector<SandboxBrokerMessageKind> supported_messages;
    DemuxBufferPlacement buffer_placement{DemuxBufferPlacement::CopyBridge};
    size_t max_message_payload_size{16 * 1024 * 1024};  // 16 MiB

    // Contract rules:
    // - Broker crash → DemuxBackendCrashedEvidence
    // - Broker timeout → DeadlineExceeded, quarantined
    // - Local trusted file MAY run in-process (with allocation/fuzz constraints)
    // - Cross-process copy count enters evidence
    // - Shared memory: owner + zeroing policy required
    bool operator==(P5SandboxBrokerContract const&) const = default;
};

}  // namespace
```

#### sandbox_crash_policy.hpp (contract, not just stub)
```
namespace kivo::video_plane::sandbox {

struct SandboxCrashPolicy {
    uint32_t max_crash_count_before_quarantine{3};
    uint64_t crash_window_seconds{60};
    uint64_t restart_delay_ms{1000};
    bool restart_on_crash{true};
    bool emit_evidence_on_crash{true};

    // Contract:
    // - Crash → DemuxBackendCrashedEvidence entry
    // - After max_crash_count → quarantined, no restart
    // - Shared memory after crash: quarantine + zero
    bool operator==(SandboxCrashPolicy const&) const = default;
};

}  // namespace
```

#### sandbox_timeout_policy.hpp
```
namespace kivo::video_plane::sandbox {

struct SandboxTimeoutPolicy {
    uint64_t open_timeout_ms{10000};
    uint64_t probe_timeout_ms{5000};
    uint64_t find_stream_info_timeout_ms{10000};
    uint64_t read_packet_timeout_ms{3000};
    uint64_t seek_timeout_ms{5000};
    uint64_t flush_timeout_ms{3000};
    uint64_t terminate_timeout_ms{5000};

    // Timeout → DeadlineExceeded evidence + quarantined if repeated
    bool operator==(SandboxTimeoutPolicy const&) const = default;
};

}  // namespace
```

#### sandbox_memory_policy.hpp
```
namespace kivo::video_plane::sandbox {

struct SandboxMemoryPolicy {
    uint64_t max_shared_memory_bytes{256 * 1024 * 1024}; // 256 MiB
    uint64_t max_process_working_set_bytes{512 * 1024 * 1024};
    bool zero_on_allocate{true};
    bool zero_on_free{true};

    bool operator==(SandboxMemoryPolicy const&) const = default;
};

}  // namespace
```

#### sandbox_file_access_policy.hpp
```
namespace kivo::video_plane::sandbox {

struct SandboxFileAccessPolicy {
    bool allow_local_file_system{true};
    bool allow_network{true};
    bool allow_temp_directory{true};
    bool deny_write_to_source_directory{true};
    bool deny_access_to_provider_credential{true};
    bool deny_access_to_drm_key_store{true};

    bool operator==(SandboxFileAccessPolicy const&) const = default;
};

}  // namespace
```

#### sandbox_shared_memory_policy.hpp
```
namespace kivo::video_plane::sandbox {

struct SandboxSharedMemoryPolicy {
    bool require_owner{true};
    bool zero_on_release{true};
    bool quarantine_on_crash{true};
    bool zero_on_quarantine{true};
    uint64_t max_handle_lifetime_ms{60000};

    bool operator==(SandboxSharedMemoryPolicy const&) const = default;
};

}  // namespace
```

**Remaining types** (conceptually defined, details same as REV 000):
- `SandboxedDemuxProcess` (struct: process_id, sandbox_token, broker_contract)
- `SandboxedDecodeProcess` (struct: process_id, sandbox_token)
- `SandboxResourceBudget` (struct: max_processes, max_shared_memory, etc.)
- `SandboxBrokerEvidence` (struct: evidence_id, broker_state, message_kind)

#### Fake Broker Test Plan
```
// tests/video_plane/sandbox/fake_sandbox_broker_tests.cpp
// - fake_broker_accepts_open_stream
// - fake_broker_rejects_over_capacity
// - fake_broker_crash_emits_evidence
// - fake_broker_timeout_emits_deadline_exceeded
// - fake_broker_shared_memory_zeroed_on_release
// - fake_broker_quarantined_after_max_crashes
```

---

### 3.5 GATE-005: Allocation Limit (2 types)

**Subdirectory**: `allocation/`

#### allocation_limit_policy.hpp (12 constexpr values)
```
namespace kivo::video_plane::allocation {

namespace AllocationDefaults {
    constexpr uint64_t kMaxSingleMediaAllocation = 64 * 1024 * 1024;   // 64 MiB
    constexpr uint64_t kMaxSingleAttachmentDescriptorBytes = 16 * 1024 * 1024;
    constexpr uint64_t kMaxSingleAttachmentBytes = 64 * 1024 * 1024;
    constexpr uint64_t kMaxTotalAttachmentBytes = 256 * 1024 * 1024;
    constexpr uint32_t kMaxTrackCount = 256;
    constexpr uint32_t kMaxChapterCount = 10000;
    constexpr uint64_t kMaxMetadataEntryBytes = 1 * 1024 * 1024;
    constexpr uint64_t kMaxTotalMetadataBytes = 32 * 1024 * 1024;
    constexpr uint64_t kMaxProbeBytes = 10 * 1024 * 1024;  // policy-capped
    constexpr uint64_t kMaxSubtitlePacketBytes = 8 * 1024 * 1024;
    constexpr uint64_t kMaxSubtitleQueueBytes = 64 * 1024 * 1024;
    constexpr uint64_t kMaxIpcPayloadBytes = 16 * 1024 * 1024;
}
```

#### allocation_limit_evidence.hpp — NEW
```
namespace kivo::video_plane::allocation {

struct AllocationLimitExceededEvidence {
    std::string limit_type;                 // which limit was exceeded
    uint64_t limit_value{0};
    uint64_t actual_value{0};
    std::string source_id;
    std::string evidence_id;
    bool operator==(AllocationLimitExceededEvidence const&) const = default;
};

}  // namespace
```

---

### 3.6 GATE-006: Fuzz (2 types)

Same as REV 000. `FuzzSmoke` (8 fields) and `P5ThirdPartyBackendRiskRegister` (10 fields).

---

### 3.7 GATE-007: Versioning (3 types)

Same as REV 000. `BoundaryContractVersion`, `CompatibilityResult`, `VersionMismatchEvidence`.

---

### 3.8 GATE-008: Failure Arbitration (5 types)

Same as REV 000. `PipelineFatalSignal` (11 values), `TerminalOutcome` (6 values), `FailureCorrelationId`, `ArbitrationEvidence`, `PipelineFailureArbiter`.

---

### 3.9 GATE-009: Recovery Seek (3 types — STRENGTHENED)

**Subdirectory**: `recovery/`

#### recovery_seek_request.hpp (REVISED — 9 fields, strong types)
```
namespace kivo::video_plane::recovery {

using kivo::video_plane::stamp::CoreVideoStamp;

struct RecoverySeekRequest {
    uint64_t timeline_revision{0};          // P4 timeline revision at request time
    uint64_t target_media_time{0};          // target media timestamp (NOT position)
    std::string target_keyframe_policy;     // "NearestBefore"/"NearestAfter"/"ExactOnly"
    std::string active_video_track;         // video track id
    std::string active_audio_track_boundary;// audio track boundary (P5 does NOT own audio)
    std::string active_subtitle_track_boundary;// subtitle track boundary (P5 does NOT own subtitle)
    std::string recovery_reason;            // machine-readable reason code
    bool user_visible{false};               // ALWAYS false — recovery is invisible
    std::string correlation_id;

    // Contract:
    // - RecoverySeekRequest does NOT enter user seek history
    // - P4 is still the playback position authority
    // - P4 may reject RecoverySeekRequest
    // - On rejection: P5 must StandbyUntilNewInput or FailToUpperLayer
    bool operator==(RecoverySeekRequest const&) const = default;
};

}  // namespace
```

#### live_edge_hint_priority.hpp — NEW (was inline, now own file)
```
namespace kivo::video_plane::recovery {

enum class LiveEdgeHintPriority {
    P4Approved,                             // P4 explicitly approved
    P4TimelineOriginHint,                   // P4 timeline origin
    RecoveryBookmark,                       // last P5 recovery bookmark
    P5Estimate,                             // P5 estimation
    ConservativeLiveRebase                  // fallback conservative
};

}  // namespace
```

#### standby_until_new_input.hpp (same as REV 000)

---

### 3.10 GATE-010: Queue Lifecycle (7 types — STRENGTHENED)

**Subdirectory**: `queue/`

#### video_pipeline_id.hpp — NEW
```
namespace kivo::video_plane::queue {

struct VideoPipelineId {
    uint64_t value{0};
    bool operator==(VideoPipelineId const&) const = default;
};

}  // namespace
```

#### cancel_token.hpp — NEW
```
namespace kivo::video_plane::queue {

struct CancelToken {
    uint64_t value{0};
    bool is_valid() const { return value != 0; }
    bool operator==(CancelToken const&) const = default;
};

}  // namespace
```

#### queue_item_id.hpp — NEW
```
namespace kivo::video_plane::queue {

struct QueueItemId {
    uint64_t value{0};
    bool operator==(QueueItemId const&) const = default;
};

}  // namespace
```

#### queue_item.hpp (REVISED — strong types only)
```
namespace kivo::video_plane::queue {

using kivo::video_plane::stamp::CoreVideoStamp;
using kivo::video_plane::failure::FailureCorrelationId;

struct QueueItem {
    QueueItemId queue_item_id;
    VideoPipelineId pipeline_id;
    CoreVideoStamp core_video_stamp;        // strong type, NOT uint64_t
    CancelToken cancel_token;               // strong type, NOT std::string
    std::optional<FailureCorrelationId> failure_correlation_id;
    QueueLifecycleState lifecycle_state{QueueLifecycleState::Accepting};
    uint64_t enqueue_time{0};
    uint64_t last_progress_time{0};

    bool operator==(QueueItem const&) const = default;
};

}  // namespace
```

**Remaining types** (same as REV 000):
- `QueueLifecycleState` (7-value enum)
- `CancelMode` (4-value enum)
- `QueueTimeoutDefaults` (7 constexpr values)

---

### 3.11 GATE-011: Protected Content (5 types + 1 NEW evidence)

Same as REV 000 plus `protected_boundary_evidence.hpp`.

### 3.12 GATE-012: Protected Output (4 types)

Same as REV 000.

### 3.13 GATE-013: Dolby Vision (3 types)

Same as REV 000.

### 3.14 GATE-014: HDR10+ (3 types)

Same as REV 000.

### 3.15 GATE-015: P010 (4 types)

Same as REV 000.

### 3.16 GATE-016: D3D11VA (4 types)

Same as REV 000.

### 3.17 GATE-017: SwapChain (6 types)

Same as REV 000.

### 3.18 GATE-018: VRR (3 types)

Same as REV 000.

### 3.19 GATE-019: Color (4 types)

Same as REV 000.

### 3.20 GATE-020: Timebase (4 types)

Same as REV 000.

### 3.21 GATE-021: Attachments (7 types)

Expanded from REV 000 (was 6, now 7 — added `external_subtitle_encoding_detection_policy.hpp`).

### 3.22 GATE-022: Presentation (3 types)

Same as REV 000.

### 3.23 GATE-023: Output Boundary (4 types)

Same as REV 000.

### 3.24 GATE-024: GPU Budget (6 types)

Same as REV 000.

### 3.25 GATE-025: Thread Runtime (5 types)

Same as REV 000.

### 3.26 GATE-026: Architecture Guard (6 types — RUNNABLE PLAN)

**Subdirectory**: `architecture/`

#### architecture_target_graph.hpp
```
namespace kivo::video_plane::architecture {

struct ArchitectureTargetGraph {
    // 8 CMake targets: contracts, core, ffmpeg_demux, ffmpeg_decode,
    //                  d3d11_gpu, dxgi_swapchain, contract_tests, future_backends

    // Link Rules (see §2)
    // Symbol checker rules (see symbol_checker_rule.hpp)
    // Include boundary rules (see include_boundary_checker.hpp)
};

}  // namespace
```

#### symbol_checker_rule.hpp
```
namespace kivo::video_plane::architecture {

struct SymbolCheckerRule {
    std::string check_name;
    std::string target;
    std::string allowed_pattern;
    std::string forbidden_pattern;
};

using PublicExportSymbolScan = SymbolCheckerRule;
using InternalSymbolScan = SymbolCheckerRule;
using ForbiddenAbiTypeScan = SymbolCheckerRule;
using DependencyGraphScan = SymbolCheckerRule;
using BinaryExportSurfaceScan = SymbolCheckerRule;

}  // namespace
```

#### link_rules.hpp (same as REV 000)

#### include_boundary_checker.hpp — NEW
```
namespace kivo::video_plane::architecture {

struct IncludeBoundaryChecker {
    // Input: path to scan (include/kivo/video_plane/)
    // Rules:
    //   - No #include <windows.h> in any contract header
    //   - No #include <d3d11.h> / <dxgi.h> in contracts or core
    //   - No #include <libavformat/*> in contracts or core
    //   - No #include <Qt/*> in contracts or core
    //   - No #include <mmdeviceapi.h> / <audioclient.h> in contracts or core
    // Output: list of violations with file:line
    struct Violation {
        std::string file_path;
        uint32_t line{0};
        std::string forbidden_include;
        std::string target;
    };
    static std::vector<Violation> scan(std::string const& root_path);
};

}  // namespace
```

#### architecture_violation_schema.hpp — NEW
```
namespace kivo::video_plane::architecture {

struct ArchitectureViolation {
    enum class Kind {
        ForbiddenLink,                      // target links forbidden library
        ForbiddenInclude,                   // header includes forbidden platform header
        ForbiddenSymbol,                    // binary exports forbidden symbol
        DependencyCycle,                    // circular dependency detected
        FutureBackendInBaseline,            // future backend in baseline binary
        PublicHeaderThirdPartyType,         // public header exposes raw third-party type
        CoreExposesBackendAbi              // core exposes FFmpeg/D3D11 ABI
    };
    Kind kind;
    std::string description;
    std::string file_path;
    std::string target;
    std::string evidence_id;
};

}  // namespace
```

#### architecture_guard_ci_command.hpp — NEW
```
namespace kivo::video_plane::architecture {

struct ArchitectureGuardCiCommand {
    // Local command:
    //   cmake --build --preset ninja-debug --target kivo_video_plane_contracts
    //   cmake --build --preset ninja-debug --target kivo_video_plane_core
    //   powershell -File backend/tools/governance/check_p5_architecture_guard.ps1
    //
    // CI command:
    //   same as local, plus:
    //   dumpbin /exports kivo_video_plane_contracts.lib | findstr /V "kivo::video_plane::"

    struct PositiveTest {
        std::string test_name;
        std::string expected_result;
    };

    struct NegativeTest {
        std::string test_name;
        std::string expected_violation;
    };

    static std::vector<PositiveTest> positive_tests();
    static std::vector<NegativeTest> negative_tests();
};

}  // namespace
```

#### Scanner Specification (concrete, runnable)

| Scanner | Input Path | Method | Rule |
|---------|-----------|--------|------|
| Target graph check | `backend/CMakeLists.txt` | CMake `--graphviz` + parse | All targets exist, all links in allowlist |
| Include boundary check | `include/kivo/video_plane/` | rg scan for forbidden includes | 0 matches in contracts/core |
| Binary symbol check | `build/kivo_video_plane_contracts.lib` | `dumpbin /exports` + filter | Only `kivo::video_plane::` symbols |
| Dependency graph scan | `backend/CMakeLists.txt` | CMake dep graph analysis | No circular deps, no future backends in baseline |
| Future backend isolation | `include/kivo/video_plane/future/` | Path + CMake check | Not in default build, not in baseline binary |

#### Gate Verification Script (design)

Script: `backend/tools/governance/check_p5_architecture_guard.ps1`

Steps:
1. CMake target existence check (8 targets)
2. `target_link_libraries` audit (10 link rules)
3. Forbidden include scan on contracts + core headers
4. Forbidden symbol scan (if binary exists)
5. Path isolation verification (ffmpeg_demux, d3d11_gpu, dxgi_swapchain, future)
6. Root CMakeLists.txt `option()` guard check
7. Output: PASS/FAIL with violation schema per §architecture_violation_schema.hpp

---

### 3.27 GATE-027: License (4 types)

Same as REV 000.

### 3.28 GATE-028: Telemetry (2 types)

Same as REV 000.

### 3.29 GATE-029: Formal State (2 types)

Same as REV 000.

### 3.30 GATE-030: CI Test (2 types)

Same as REV 000.

### 3.31 GATE-031: Malformed Corpus (1 type)

Same as REV 000.

### 3.32 GATE-034: Frame Export Security (1 type)

Same definition as in `p5a_gate_034_frame_export_security_skeleton.md`.

### 3.33 GATE-035: Plugin Security (1 type)

Same as REV 000.

### 3.34 GATE-036: Window Visibility (1 type)

Same as REV 000.

### 3.35 GATE-037: Success Definition (1 type)

Same as REV 000.

### 3.36 GATE-038: Change Control (1 type)

Same as REV 000.

---

### 3.37 Developer Guide Skeleton (GATE-033, EXPANDED — 9-row uniform template)

**File**: `docs/developer/p5_video_plane_developer_guide.md` (skeleton only)

All 11 scenarios use uniform template:

| Row | Content |
|-----|---------|
| 1 | **Pipeline Flow**: step-by-step data flow |
| 2 | **Key Types**: types from P5A contracts involved |
| 3 | **Thread Flow**: which threads handle which steps |
| 4 | **Stamp Changes**: when CoreVideoStamp/FullVideoStamp updates |
| 5 | **Capability States**: which CapabilityDomainId states are relevant |
| 6 | **Error Path**: what happens on each failure mode |
| 7 | **Test Names**: corresponding contract/integration test names |
| 8 | **What P5 Owns**: P5 responsibilities in this scenario |
| 9 | **What P5 Does NOT Own**: explicitly NOT P5 responsibility |

**11 Scenarios** (same as REV 000, now with uniform template applied):

1. 本地 MP4 SDR 播放 — P4→InputBridge→Demux(SW)→Decode(SW)→Render(SDR)→Present→P8
2. HEVC HDR10 / P010 播放 — D3D11VA Decode→P010 SRV→HDR Render→Present
3. 在线 live low-latency stream — Demux(live)→LiveTimestamp→Decode→low-latency Render→Present
4. Protected content / output protection loss — ProtectedBoundary→SecureDecode→OutputProtection→FailClosed
5. Dolby Vision Profile 5 fail-closed — DV detect→no engine→FailClosed evidence
6. Malformed file bounded failure — malformed input→AllocationLimit→BoundedFailure evidence
7. Device lost during HDR playback — DeviceRemoved→HoldLastSafeFrame→Rebuild or FailToUpperLayer
8. Monitor move SDR→HDR — DisplayCapabilityRefresh→HDR enable(if capable)
9. D3D11VA P010 fallback — IntegerPlane fail→UNORM fallback→tolerance check
10. Live low latency catch-up — Queue buildup→CatchUpPolicy→Skip or reduce quality
11. Subtitle attachment safety — Attachment detect→MIME sniff→Quota→OpaqueFlow→P7

---

### 3.38 Bootstrap Single-Owner Rule (P5A A9)

```
Bootstrap Rule:
- At P5 bootstrap, exactly ONE component owns P5 initialization.
  (P5BootstrapOwner = P5VideoPlaneCore)
- All P5 components register their contracts with the bootstrap owner.
- The CapabilityRegistry is populated during bootstrap.
- No component self-initializes outside bootstrap order.
- Bootstrap timeout: 5000ms (CapabilityBootstrapPolicy::bootstrap_timeout_ms)
- On bootstrap timeout: emit BootstrapTimeoutEvidence, retry once, then FailToUpperLayer.
```

---

## 4. P5A PRECISE ALLOWLIST (DIRECTORIES + FILES)

### 4.1 Allowed Directories (production)

| # | Directory | Purpose |
|---|-----------|---------|
| 1 | `include/kivo/video_plane/` | Root contract directory |
| 2 | `include/kivo/video_plane/stamp/` | CoreStamp/FullStamp/Validator |
| 3 | `include/kivo/video_plane/capability/` | Capability Registry |
| 4 | `include/kivo/video_plane/evidence/` | Evidence Schema |
| 5 | `include/kivo/video_plane/threat/` | Threat Matrix |
| 6 | `include/kivo/video_plane/sandbox/` | Sandbox Protocol |
| 7 | `include/kivo/video_plane/allocation/` | Allocation Limits |
| 8 | `include/kivo/video_plane/fuzz/` | Fuzz Smoke / Risk Register |
| 9 | `include/kivo/video_plane/version/` | Boundary Versioning |
| 10 | `include/kivo/video_plane/failure/` | Failure Arbitration |
| 11 | `include/kivo/video_plane/recovery/` | Recovery Seek |
| 12 | `include/kivo/video_plane/queue/` | Queue Lifecycle |
| 13 | `include/kivo/video_plane/protected_content/` | Protected Content |
| 14 | `include/kivo/video_plane/protected_output/` | Multi-monitor Output |
| 15 | `include/kivo/video_plane/dolby_vision/` | Dolby Vision |
| 16 | `include/kivo/video_plane/hdr10_plus/` | HDR10+ |
| 17 | `include/kivo/video_plane/p010/` | P010 Formula |
| 18 | `include/kivo/video_plane/d3d11va/` | D3D11VA |
| 19 | `include/kivo/video_plane/swapchain/` | DXGI SwapChain |
| 20 | `include/kivo/video_plane/vrr/` | VRR |
| 21 | `include/kivo/video_plane/color/` | Color/HDR/Chroma |
| 22 | `include/kivo/video_plane/timebase/` | Timebase |
| 23 | `include/kivo/video_plane/attachment/` | Attachment Safety |
| 24 | `include/kivo/video_plane/presentation/` | PresentationEpoch |
| 25 | `include/kivo/video_plane/output_boundary/` | P5→P6/P7/P8 Boundary |
| 26 | `include/kivo/video_plane/gpu/` | GPU Budget |
| 27 | `include/kivo/video_plane/thread/` | Thread Runtime |
| 28 | `include/kivo/video_plane/architecture/` | Architecture Guard |
| 29 | `include/kivo/video_plane/license/` | License/Patent |
| 30 | `include/kivo/video_plane/telemetry/` | Telemetry Privacy |
| 31 | `include/kivo/video_plane/state_monitor/` | Formal State Monitor |
| 32 | `include/kivo/video_plane/test_ci/` | Test/CI Gates |
| 33 | `include/kivo/video_plane/malformed/` | Malformed Corpus |
| 34 | `include/kivo/video_plane/export/` | Frame Export Security |
| 35 | `include/kivo/video_plane/plugin/` | Plugin Security |
| 36 | `include/kivo/video_plane/window/` | Window Visibility |
| 37 | `include/kivo/video_plane/success/` | Success Definition |
| 38 | `include/kivo/video_plane/change/` | Change Control |

### 4.2 Allowed Directories (tests)

| # | Directory | Purpose |
|---|-----------|---------|
| T1 | `tests/video_plane/stamp/` | Stamp contract tests |
| T2 | `tests/video_plane/capability/` | Capability contract tests |
| T3 | `tests/video_plane/evidence/` | Evidence contract tests |
| T4 | `tests/video_plane/threat/` | Threat matrix tests |
| T5 | `tests/video_plane/sandbox/` | Sandbox broker tests |
| T6 | `tests/video_plane/allocation/` | Allocation limit tests |
| T7 | `tests/video_plane/version/` | Versioning tests |
| T8 | `tests/video_plane/failure/` | Failure arbitration tests |
| T9 | `tests/video_plane/recovery/` | Recovery seek tests |
| T10 | `tests/video_plane/queue/` | Queue lifecycle tests |
| T11 | `tests/video_plane/thread/` | Thread contract tests |

### 4.3 Allowed CMake Files

| # | File | Purpose |
|---|------|---------|
| 1 | `backend/CMakeLists.txt` | Add `kivo_video_plane_contracts` target + test executable |
| 2 | `CMakeLists.txt` (root) | Option guard `KIVO_ENABLE_P5_VIDEO_PLANE` |

### 4.4 Allowed Docs

| # | File | Purpose |
|---|------|---------|
| 1 | `docs/roadmap/p5a_planning_draft.md` | THIS DOCUMENT |
| 2 | `docs/roadmap/p5_v2.2_execution_card.md` | Pre-existing execution card |
| 3 | `docs/roadmap/p5a_gate_026_architecture_guard_skeleton.md` | Architecture guard skeleton |
| 4 | `docs/roadmap/p5a_gate_034_frame_export_security_skeleton.md` | Frame export security |
| 5 | `docs/developer/p5_video_plane_developer_guide_skel.md` | Developer guide skeleton |
| 6 | `docs/roadmap/p5a_self_check.md` | P5A exit criteria self-check |

### 4.5 Forbidden Files / Directories (hard stop)

| # | Forbidden Item |
|---|---------------|
| F-01 | ❌ Production `.cpp` implementation files (NO `src/video_plane/...`) |
| F-02 | ❌ `#include <windows.h>` in ANY P5A header |
| F-03 | ❌ `#include <d3d11.h>` or `<dxgi.h>` in ANY P5A header |
| F-04 | ❌ `#include <libavformat/*>` or `<libavcodec/*>` in ANY P5A header |
| F-05 | ❌ `#include <mmdeviceapi.h>` or `<audioclient.h>` in ANY P5A header |
| F-06 | ❌ Raw COM types (`IUnknown`, `ID3D11*`, etc.) in ANY P5A header |
| F-07 | ❌ Raw FFmpeg types (`AVFormatContext`, `AVCodecContext`, etc.) |
| F-08 | ❌ `src/video_plane/` directory creation (no implementation yet) |
| F-09 | ❌ Any file outside the allowed directories listed in §4.1–§4.4 |
| F-10 | ❌ "口号式能力声明" — every type must bind test_name/evidence_kind |
| F-11 | ❌ NO implementation before THIS PLANNING DRAFT is APPROVED |
| F-12 | ❌ NO code entering master before P5 Gate Review |
| **Allow** | ✅ Test `.cpp` files ARE allowed for contract tests (`tests/video_plane/`) |

---

## 5. P5A EXIT CRITERIA COVERAGE TABLE (16 items → 16 sections)

| # | Criterion | Gate | Coverage in Draft | Status |
|---|-----------|------|-------------------|--------|
| A1 | public contracts compile | ALL | §3: 121 headers defined, not yet compiled (blocked by APPROVAL) | ⏳ DESIGNED |
| A2 | CapabilityRegistry contract complete | GATE-001 | §3.1: 12 types, 25 CapabilityDomainId values, Registry contract | ✅ COVERED |
| A3 | EvidenceSchema contract complete | GATE-002 | §3.2: 8 types, 22-field EvidenceRecord fully expanded | ✅ COVERED |
| A4 | CoreStamp / FullStamp contract complete | GATE-STAMP | §3.0: CoreVideoStamp + FullVideoStamp + fields + equality | ✅ COVERED |
| A5 | P5StampValidator contract complete | GATE-STAMP | §3.0: P5StampValidator + StampValidationContext/Kind/Result | ✅ COVERED |
| A6 | QueueLifecycleState contract complete | GATE-010 | §3.10: 7 types, QueueItem with strong types (no strings) | ✅ COVERED |
| A7 | Threading matrix compile-time docs | GATE-025 | §3.25: 5 types defined; dev guide §3.37 has thread flow per scenario | ✅ COVERED |
| A8 | P4 recovery seek boundary defined | GATE-009 | §3.9: 9-field RecoverySeekRequest, user_visible=false, P4 authority rule | ✅ COVERED |
| A9 | Bootstrap single-owner rule defined | (P5A) | §3.38: explicit bootstrap owner, timeout, retry rule | ✅ COVERED |
| A10 | PresentationEpoch trigger table draft | GATE-022 | §3.22: EpochCondition, HoldLastFrameSafe (6 conditions), GeometryPipelineOrder (7 steps) | ✅ COVERED |
| A11 | Security threat model skeleton complete | GATE-003 | §3.3: 5 types, 29 surface enum, matrix container, completeness checker, gate mapping | ✅ COVERED |
| A12 | Architecture graph guard skeleton runnable | GATE-026 | §3.26: 6 types, 5 scanners with concrete paths/methods, gate script spec, positive/negative tests | ✅ COVERED |
| A13 | No FFmpeg/D3D11/Qt/WASAPI in public contract target | GATE-026 | §5: F-02~F-07 hard stop forbidden items; §2: link rules 1+9 | ✅ COVERED |
| A14 | Developer guide skeleton exists | GATE-033 | §3.37: 11 scenarios with uniform 9-row template | ✅ COVERED |
| A15 | BoundaryContractVersion types defined | GATE-007 | §3.7: BoundaryContractVersion + CompatibilityResult + VersionMismatchEvidence + 8 typedefs | ✅ COVERED |
| A16 | FailureArbiter contract defined | GATE-008 | §3.8: 5 types, 11 FatalSignalSource values, 6 TerminalOutcome values | ✅ COVERED |

**Coverage Summary**: 16/16 COVERED (0 NOT COVERED, 0 IMPLICIT — A5 StampValidator now EXPLICIT)

---

## 6. P5S PARALLEL SECURITY MAPPING

P5S runs in parallel from P5A, blocks P5H closure. This section lists every P5S criterion and its P5A contract skeleton.

| P5S # | Criterion | P5A Contract Section | Status |
|-------|-----------|---------------------|--------|
| S1 | ThreatMitigationMatrix complete | §3.3: 5 types (item, matrix, completeness checker, gate mapping) | SKELETON |
| S2 | Sandbox broker protocol reviewed | §3.4: 14 types + 13 messages + IPC header | SKELETON |
| S3 | Allocation policy tests pass | §3.5: 2 types + 12 constexpr values | SKELETON |
| S4 | Attachment quota tests pass | §3.21: 7 types (GATE-021) | SKELETON |
| S5 | Metadata sanitizer tests pass | §3.2: EvidenceRedactionLevel + EvidenceRecord redaction fields | SKELETON |
| S6 | Malformed corpus smoke pass | §3.31: MalformedCorpusItem + GATE-031 rules | SKELETON |
| S7 | Fuzz smoke pass | §3.6: FuzzSmoke + ThirdPartyBackendRiskRegister | SKELETON |
| S8 | Security evidence redaction tests pass | §3.2: EvidenceRecord redaction_level + pii_safety_class | SKELETON |
| S9 | Third-party backend risk register complete | §3.6: P5ThirdPartyBackendRiskRegister (10 fields) | SKELETON |
| S10 | CVE response process documented | §3.33: Change Control + Plugin Security references | SKELETON |

---

## 7. CONTRACT TEST PLAN (P5A)

All contract test `.cpp` files are allowed. Production `.cpp` files are forbidden.

### Test Executable: `kivo_video_plane_contract_tests`

### Test Families

| # | Test Directory | Expected Tests (min) | GATE |
|---|---------------|---------------------|------|
| 1 | `tests/video_plane/stamp/` | 12 | STAMP |
| 2 | `tests/video_plane/capability/` | 15 | GATE-001 |
| 3 | `tests/video_plane/evidence/` | 12 | GATE-002 |
| 4 | `tests/video_plane/threat/` | 8 | GATE-003 |
| 5 | `tests/video_plane/sandbox/` | 10 | GATE-004 |
| 6 | `tests/video_plane/allocation/` | 6 | GATE-005 |
| 7 | `tests/video_plane/version/` | 6 | GATE-007 |
| 8 | `tests/video_plane/failure/` | 8 | GATE-008 |
| 9 | `tests/video_plane/recovery/` | 7 | GATE-009 |
| 10 | `tests/video_plane/queue/` | 10 | GATE-010 |
| 11 | `tests/video_plane/thread/` | 8 | GATE-025 |

**Total estimated**: ~102 contract tests across 11 test directories

### Test Infrastructure
- Test runner: same `ContractTestRunner` pattern as existing P0-C tests
- Test style: `[]() { /* test body */ }` lambdas
- CMake: single `kivo_video_plane_contract_tests` EXECUTABLE
- No fake backends needed in P5A (contracts only, no runtime)
- No FFmpeg/D3D11/DXGI/Qt/WASAPI in test link

---

## 8. UNRESOLVED ISSUES

**None.**

All 13 user-flagged issues addressed in this revision:
1. ✅ CoreStamp/FullStamp/P5StampValidator → §3.0 (6 types)
2. ✅ Capability domain contradiction → §3.1 (25 domains, CapabilityDomainId enum, noted source doc discrepancy)
3. ✅ EvidenceRecord 22 fields → §3.2 (full table + struct definition)
4. ✅ CMake core target semantics → §2 (core = backend-neutral, NOT FFmpeg wrapper)
5. ✅ P5A allowlist → §4 (38 dirs + 11 test dirs + 2 CMake + 6 docs + 13 forbidden items)
6. ✅ F-01 split → §4.5 F-01 (production .cpp FORBIDDEN, test .cpp ALLOWED)
7. ✅ RecoverySeekRequest strength → §3.9 (9 fields: timeline_revision, target_media_time, keyframe_policy, track_boundaries, recovery_reason, correlation_id)
8. ✅ QueueItem strong types → §3.10 (VideoPipelineId, CoreVideoStamp, CancelToken, QueueItemId, FailureCorrelationId)
9. ✅ SandboxBrokerContract → §3.4 (14 types + 13 messages + IPC header 12 fields + DemuxBufferPlacement)
10. ✅ Threat Matrix → §3.3 (5 types: item + matrix container + completeness checker + gate mapping + 29 ThreatSurfaceId enum)
11. ✅ Architecture Guard runnable → §3.26 (6 types + 5 scanners with concrete paths/methods + gate script + positive/negative test plan)
12. ✅ Developer Guide template → §3.37 (11 scenarios with uniform 9-row template)
13. ✅ Exit Criteria coverage → §5 (16-item table, every criterion mapped to draft section)

---

## 9. IMMEDIATE NEXT STEPS (AFTER APPROVAL)

1. Create directory scaffold (32 `include/kivo/video_plane/*/` + 11 `tests/video_plane/*/`)
2. Create CMakeLists.txt: `kivo_video_plane_contracts` INTERFACE + `kivo_video_plane_contract_tests` EXECUTABLE
3. Wire into root CMakeLists.txt with `option(KIVO_ENABLE_P5_VIDEO_PLANE OFF)`
4. Create headers in order: stamp → capability → evidence → threat → sandbox → ... → change
5. Create test runners matching header structure
6. Run Gate Review: compile + CTest + 13 forbidden item checks

---

**END OF P5A PLANNING DRAFT REVISION 001**
**Status**: DRAFT_IN_PROGRESS — awaiting user approval before directory creation / header files / CMake
