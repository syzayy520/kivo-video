# KIVO-VIDEO-P7-PLAYBACK-GRAPH-INTEGRATION-DESIGN-LOCK-001-V1.9

**Source DOCX**: `C:/Users/huyasi/Downloads/Kivo Video P7 Playback Graph Integration Design V1.9.docx`
**Source SHA256**: `B85D3B4C200445A46469AD4600B97BD0BB409F0DA7D3679BA1E0E44F87529FD8`
**Extracted**: `2026-06-29T01:03:34Z`
**Paragraph count**: `1108`
**Extraction method**: OOXML `word/document.xml` paragraph text extraction; no semantic rewrite.

> This file is the project-local V1.9 baseline used only for P7 V1.9.1 carryover audit. P7 implementation remains based on V1.9.1.

## Kivo Video P7 Playback Graph Integration 终极设计书 V1.9

Task ID:KIVO-VIDEO-P7-PLAYBACK-GRAPH-INTEGRATION-DESIGN-LOCK-001-V1.9

Document Type:architecture-design / playback-graph-integration / implementation-baseline / p2-p3-p4-p5-p6-bridge / p4-state-sync-adapter-lock / p4-revision-owner-lock / p7-command-intent-sidecar / async-result-lifecycle-lock / command-lifetime-registry / nonblocking-token-only-api / generation-stamp-opaque-id-lock / seqlock-retry-budget-lock / stamp-read-failure-drop-scope-lock / no-128bit-atomic-lock / versioned-critical-event-slot / no-critical-event-aba / single-consumer-critical-event-lock / hot-path-thread-definition-lock / hot-path-static-runtime-guard / async-attachment-token-contract / operation-id-uniqueness-lock / best-effort-cancel-contract / phase-bitset-contract / seek-phase-timeout-contract / seek-logical-completion-lock / recycle-nonblocking-seek / resource-retire-queue / closed-through-closing-lock / failed-fast-closing-contract / rebuffering-pause-timeout-contract / hold-readiness-contract / contract-gap-scanner / contract-existence-tooling / command-conflict-matrix-codegen / rebuild-predicate-codegen / performance-budget-tool / queue-budget-explicit-dimension-mode / micro-adjust-capability-refresh / architecture-guard / public-header-purist-check / private-test-support-only / no-ui / no-subtitle-runtime / no-new-decoder / no-new-output / no-provider-redesign

Status:DESIGN_BASELINE_FOR_IMPLEMENTATION

### 0. 最终执行口令

P7 不是“再写一个播放器核心”。

P7 只做一件事：

把 P2/P3 Source Runtime、P4 Playback Core、P5 Video Media Plane、P6 Audio Media Plane接成一个统一、可调度、可 seek、可 pause/resume、可 track switch、可 buffering/rebuffering/stalled、可 EOS 后 seek、可 domain-isolated recovery、可 physical resource retire/recycle、可 evidence/metrics 追踪、可 deterministic fake 测试、可安全 close、可处理 late callback / late recycle ack、可承受系统休眠恢复、设备变化、网络重连、高频 seek、GPU recycle 延迟的 Playback Graph。

P7 完成后，后端状态必须从：

P4/P5/P6 modules completed but not integrated.

变成：

Playback graph integrated.One PlaybackSession can drive source + video + audio as one coordinated graph.

V1.9 锁定以下硬规则：

1. P7→P4 状态同步唯一方案：P4StateSyncAdapter。2. PlaybackRevision 只由 P4 分配，P7 不得生成、递增、伪造 PlaybackRevision。3. P4StateSyncAdapter 只做纯语义校验与 PlaybackRevision 返回，不查询 P5/P6/source 物理状态。4. P4 frozen command header 不修改：P7CommandIntentSidecar 绑定 PlaybackCommandId。5. AsyncResult 是非阻塞 command token，不允许 get/wait/blocking_result。6. AsyncResult 生命周期由 PlaybackSession/GraphOwnerThread 管理，hot path 不访问 command registry。7. Sidecar、AsyncResult、PendingCommand、CompletionEvent 均以 PlaybackCommandId / GraphCorrelationId 关联。8. GenerationStamp 唯一方案：opaque per-stream stamp id + immutable GenerationTupleRegistry + seqlock publication。9. GenerationTuple 不使用 128-bit atomic；只使用 lock-free 64-bit atomic/seqlock 字段。10. Seqlock 读取最多 retry 3 次，失败后 drop 当前未提交 frame/packet，emit StampReadUnstable evidence。11. 已提交给 renderer/device 的 frame/packet 不由 P7 回收，由 P5/P6 自己做 generation check。12. Critical event 使用 versioned slot / ring，禁止 exchange(0) 清空整组 flag。13. Critical event 允许多生产者，但只允许 GraphOwnerThread 单消费者。14. Hot Path 线程集合明确：AudioOutputThread、VideoPresentationThread、P5/P6 高频回调线程。15. Hot path 禁止 allocation、blocking、logging、mutex、future wait、文件 IO、网络 IO、sleep。16. P5/P6 Attachment 耗时操作统一 async token + completion event。17. Attachment operation id 必须以 AttachmentKind + operation_id 构成唯一 OperationKey。18. Cancel 是 best-effort、幂等；timeout 后 late completion 必须按 tombstone 处理。19. Phase Bitset 是 async phase 唯一同步机制，所有 phase 必须有 timeout。20. Seek 逻辑完成不等待旧 resource recycle；旧资源进入 retire queue 后后台 recycle。21. Recycle 只在 surface_pool_exhausted / synchronous_recycle_required / protected path / retire queue full 时阻塞 seek。22. 所有 Closed 转移必须经过 Closing。23. Failed / BuildFailed / NotCreated / Created 进入 Closing 时允许 fast closing path，但必须发布 final snapshot/evidence。24. Recovery pending replay 失败必须 abort 后续 replay，并返回 typed result。25. Rebuffering pause intent 超时后的状态转移必须明确。26. Queue budget 支持显式禁用单个维度，但禁止使用 raw zero 产生隐式半配置歧义。27. Contract Gap 必须由 Contract Gap Scanner + Header Diff Tool + Behavior Test Suite 支持。28. Command Conflict Matrix 必须代码化，测试从同一 source-of-truth 生成。29. Rebuild Granularity Predicate 必须代码化，禁止文档和代码两套规则漂移。30. Public include 不允许暴露 fake/test_support/test_contract。31. Public Header Purist Check 必须进入 CI。32. No UI / no subtitle runtime / no new decoder / no new output / no provider redesign。

### 1. Merge Integrity Gate

V1.9 必须被视为 V1.8 的严格超集。

实现前必须确认：

1. V1.8 中所有 P7 边界、禁止事项、状态机、epoch/generation、GenerationStamp、seek、pause/resume、track switch、EOS、recovery、evidence、metrics、fake scheduler、architecture guard、STOP conditions、Final Report 要求均未删除。2. V1.9 中新增内容只能加强或细化 V1.8，不得弱化 V1.8 的任何硬规则。3. 如果 V1.8 与 V1.9 命名不同，以 V1.9 新命名为准，但语义必须保持覆盖。4. 如果实现者发现 V1.8 某条规则在 V1.9 中找不到对应项，必须 STOP，输出 Missing-Carryover Report。

Missing-Carryover Report 必须列出：

v1_8_original_itemv1_9_missing_or_renamed_locationimpact_scopesuggested_restore_locationbaseline_commitworking_tree_diff_summaryfiles_modified_before_stop

### 2. Platform / Toolchain / Language Baseline

2.1 目标平台

Primary target: Windows desktop.UI future target: Qt 6 desktop runtime adapter in P9.P7 itself: no Qt dependency.

2.2 默认工程路径

默认工程根目录：

{KIVO_VIDEO_ROOT} = C:\KivoVideo

禁止默认使用：

C:\kivo video

如果真实仓库已经在带空格路径中，P7 任务必须在 Pre-flight Report 中明确：

space_path_detected: truequoted_path_validation: PASS

2.3 C++ 标准与编译器

P7 默认基线：

C++20MSVC 2022 v143 or newerWindows x64

规则：

1. 不依赖 std::expected，避免 C++23 绑定。2. 使用项目自定义 Result<T, E> / StatusOr<T> 风格返回 typed failure。3. public ABI 不跨 DLL 暴露 STL 容器，除非项目已有稳定 ABI 规则。4. noexcept 只用于保证不抛异常的函数；noexcept 函数内部不得依赖可能抛异常的动态分配。5. P7 hot path 不允许异常传播。6. 所有 public enum class 必须显式指定 uint32_t underlying type。7. 所有跨 ABI opaque handle 使用 std::uintptr_t 或项目统一 handle 类型。

### 3. Scope / Non-goals

3.1 P7 允许做

P7 允许新增：

PlaybackGraphPlaybackSessionPlaybackGraphBuilderPlaybackGraphTopologyPlaybackGraphStateMachineGraphOrchestratorGraphOperationCoordinatorP4StateSyncAdapterP7CommandIntentSidecarP4RevisionBridgePlaybackGraphCommandRouterPlaybackGraphCommandQueuePendingGraphCommandQueuePlaybackGraphCommandCoalescerPlaybackGraphCommandBarrierCommandLifetimeRegistryCommandConflictMatrixSourceCommandConflictMatrixGeneratorPlaybackGraphPolicyGraphTuningParametersPolicyPresetSafetyCriticalTuningValidatorQueueBudgetDimensionPolicyPerformanceBudgetToolSourcePipelineAttachmentDemuxPipelineAttachmentAudioPipelineAttachmentVideoPipelineAttachmentAttachmentAsyncTokenAttachmentCompletionEventAttachmentCompletionTimerAttachmentOperationRegistryAttachmentOperationTombstoneSeekFlushCoordinatorDrainCancelCoordinatorPauseResumeCoordinatorTrackSwitchCoordinatorBufferingCoordinatorRebufferingCoordinatorStalledCoordinatorEosDrainCoordinatorCloseCoordinatorPhaseBitsetPhaseTimeoutPolicyGraphClockCoordinatorAVSyncDecisionDeliveryGraphRecoveryCoordinatorSecondaryDomainVerifierSourceResumeVerifierGraphRecoveryFuseGraphResourceRetireQueueGraphResourceRecycleCoordinatorRecycleBarrierGenerationStampGenerationTupleRegistryGenerationStampPublisherSeqlockStampPublisherRealtimeEventAllocatorVersionedCriticalEventSlotCriticalEventRingGraphEvidenceCollectorGraphMetricsCollectorGraphEventSinkGraphHealthSnapshotSystemEventIngressContractGapReportContractGapRegistryContractGapScannerContractHeaderDiffToolContractBehaviorTestSuitePublicHeaderPuristCheckArchitectureGuardMissingCarryoverReport

3.2 P7 禁止做

P7 禁止：

1. 禁止新增真实 FFmpeg decode。2. 禁止新增真实 WASAPI output。3. 禁止新增真实 video renderer。4. 禁止新增字幕解析。5. 禁止新增字幕渲染。6. 禁止新增 Qt/UI。7. 禁止新增 provider。8. 禁止重写 Source Runtime。9. 禁止重写 Demux。10. 禁止重写 P4 command/state/recovery。11. 禁止修改 P4/P5/P6 frozen public contract。12. 禁止修改 P2/P3 source frozen public contract。13. 禁止把 FFmpeg/WASAPI/Qt/D3D/DXGI/Win32/COM 类型泄漏到 public graph headers。14. 禁止 UI 直接调用 P5/P6。15. 禁止绕过 P4 command/state/recovery 模型。16. 禁止把 P7 写成 PlaybackManager 大桶。17. 禁止 hash 型 GenerationStamp。18. 禁止 hot path allocation。19. 禁止 hot path blocking。20. 禁止 hot path logging。21. 禁止 hot path mutex。22. 禁止 hot path future wait。23. 禁止 hot path 文件 IO。24. 禁止 hot path 网络 IO。25. 禁止 hot path sleep。26. 禁止 AudioOutputThread 或 VideoPresentationThread 因任何 queue full 阻塞。27. 禁止 public include 暴露 fake/test_support/test_contract。28. 禁止 GraphOwnerThread 阻塞等待 attachment future。29. 禁止 recovery pending command 静默丢弃。30. 禁止任何状态直接跳 Closed，必须经过 Closing。31. 禁止 critical event 用 exchange(0) 清空整组 flag。32. 禁止 critical event 多消费者。33. 禁止 seqlock 读取失败后继续渲染当前未提交帧或输出当前未写入音频包。34. 禁止 seek 常规路径硬等待旧 resource recycle 完成。35. 禁止 P7 生成或递增 PlaybackRevision。

### 4. Terminology

GraphOwnerThread:  P7 唯一主控线程。负责状态转移、命令分发、phase 推进、recovery 协调、snapshot 发布。Hot Path:  直接参与音频输出、视频呈现、设备回调、高频媒体回调的实时路径。Attachment:  P7 对 P2/P3/P5/P6/Demux 的异步操作边界。所有耗时操作必须 token + completion event。GenerationStamp:  用于 hot path O(1) 判断数据是否属于当前 generation 的不透明版本戳。GenerationTuple:  slow path evidence 用于展开 GenerationStamp 的完整 generation/epoch 组合。Seqlock:  GraphOwnerThread 写、hot path 读的无锁发布机制。读取失败必须 drop 当前未提交数据。Critical Event:  hot path 产生的必须被 GraphOwnerThread 观察到的关键事件，例如 fatal underrun、stamp unstable、device lost、queue corruption。CommandLifetimeRegistry:  GraphOwnerThread 私有 command 生命周期表。hot path 不得访问。OperationKey:  AttachmentKind + AttachmentOperationId 的复合 key，用于唯一标识 async attachment operation。PhaseBitset:  一个异步操作阶段内多个 required operation 的完成集合，用于 event-driven phase 推进。

禁止使用：

ControlThreadManagerThreadPlaybackManager

统一使用：

GraphOwnerThreadGraphOrchestratorCoordinator

### 5. Pre-flight Checks

Pre-flight 是施工前提，不等同于设计 STOP。

开工前必须确认：

git rev-parse --show-toplevelgit branch --show-currentgit status --short

要求：

### 1. 当前 repo 必须是 {KIVO_VIDEO_ROOT} 或项目明确登记的真实根目录。2. 当前分支必须是 P7 专用分支。3. 工作区必须 clean。4. 禁止在 main/master 直接施工。5. 禁止夹带 P8/P9/P10 文件。

建议分支名：

kivo-video-p7-playback-graph-integration

Pre-flight 不通过时：

PREFLIGHT_FAIL不得开始实现不得输出 Contract Gap Report必须先修复工作区/分支/目录问题

### 6. Contract Gap Handling / Registry

6.1 Contract Gap 触发条件

如果 P7 需要 P2/P3/P4/P5/P6 提供某个合同，而该合同不存在、不清晰、未 frozen、只有 fake 无真实、只有 stub 无语义、fake 与真实路径语义冲突、或无法通过 contract tooling 验证，必须触发 Contract Gap。

典型 gap：

P2/P3 SourceOperationToken 缺失P2/P3 source resume/reconnect/reopen state query 缺失P4 stable PlaybackCommandId 缺失P4 state sync contract 缺失P4 revision validation contract 缺失P5 async attachment token/completion event 缺失P5 normalized timeline stamp 缺失P5 physical resource retire/recycle contract 缺失P5 LastFramePresented / PipelineDrained event 缺失P5 AVSync decision application contract 缺失P5 lock-free or bounded queue snapshot 缺失P6 async attachment token/completion event 缺失P6 OutputDrained / PipelineDrained event 缺失P6 underrun/silence-fill capability 缺失且无 typed fallbackP6 lock-free or bounded queue duration snapshot 缺失P6 micro-adjust capability 缺失但 policy 要求启用

6.2 Contract Existence Criterion

一个合同只有同时满足以下条件才算存在：

1. public contract header 存在。2. contract 描述输入、输出、生命周期、错误语义、线程语义。3. fake implementation 存在并可用于 P7 tests。4. real backend path 有实现或明确 declared unsupported typed result。5. fake 与 real 的结构一致：方法签名、返回类型、错误类型、operation id、event type 完全一致。6. fake 与 real 的语义一致：相同输入产生相同输出、相同错误类别或同等 unsupported typed result。7. fake 与 real 的线程语义一致：非阻塞、bounded read、callback thread、completion event 规则一致。8. CTest 或 architecture guard 能证明 fake 与真实路径的合同形态一致。9. 不是空 stub。10. 不要求 P7 引入 forbidden dependency。11. 如果是 async contract，必须有 token、completion event、cancel semantics、timeout semantics。12. 如果是 snapshot contract，必须定义 lock-free 或 bounded critical section 读取语义。

6.3 Contract Tooling

P7 必须提供或接入：

ContractGapScannerContractHeaderDiffToolContractBehaviorTestSuite

ContractHeaderDiffTool 至少检查：

method nameargument typereturn typeerror enumoperation id typecompletion event typethreading annotationnoexcept / Result style

ContractBehaviorTestSuite 至少检查：

success pathunsupported typed resulttimeout pathcancel pathlate completion pathduplicate completion pathwrong generation stamp pathbounded snapshot read path

6.4 Contract Gap Registry 格式

Registry 必须使用可机器解析结构：

gap_id: P7-GAP-0001status: openowner_phase: P5missing_symbol: VideoPipelineAttachment::LastFramePresentedrepair_task_id: KIVO-VIDEO-P5-REPAIR-LAST-FRAME-PRESENTED-001created_by: p7_contract_gap_scannercreated_at: 2026-xx-xxblocking_p7_tests:  - test_name: eos_waits_last_frame_presented    block_type: missing_contract    reason: P5 does not expose LastFramePresented event  - test_name: eos_waits_output_drained    block_type: dependent_contract    reason: EOS drain cannot prove final frame/audio output completedverification_commit: nullclosed_at: null

blocking_p7_tests 必须包含：

test_nameblock_typereason

6.5 STOP 触发时机

Contract Gap STOP 可在以下时机触发：

触发时机

检查方式

工具

configure

public contract header 缺失

CMake configure

compile

type/signature 缺失

compiler

link

fake/test support implementation 缺失

linker

test

behavior mismatch

CTest / Behavior Test Suite

scan

P7 依赖不存在或 forbidden

Contract Gap Scanner

review

人工发现合同语义缺失

Contract Gap Report

自动检测到 gap 后必须：

### 1. STOP。2. 写 Contract Gap Registry。3. 输出 Contract Gap Report。4. 不允许在 P7 偷补真实 P2/P3/P4/P5/P6 能力。

6.6 Contract Gap Report

STOP 时必须输出：

gap_idmissing_symbolexpected_contractowner_phase: P2/P3/P4/P5/P6blocking_pathwhy_P7_cannot_implement_itrisk_if_workaround_usedallowed_temporary_test_fake_scopesuggested_repair_task_idfiles_inspectedbaseline_commitworking_tree_diff_summaryfiles_modified_before_stop

### 7. Natural Family Tree

7.1 Public include family

Public include 只放产品合同，禁止 fake/test_support。

include/kivo/video/playback_graph/│├── contracts/│   ├── playback_graph_id.hpp│   ├── playback_graph_state.hpp│   ├── playback_graph_error.hpp│   ├── playback_graph_result.hpp│   ├── playback_graph_generation.hpp│   ├── generation_stamp.hpp│   ├── generation_tuple_registry.hpp│   ├── generation_stamp_publisher.hpp│   ├── seqlock_stamp_publisher.hpp│   ├── realtime_event_allocator.hpp│   ├── versioned_critical_event_slot.hpp│   ├── playback_graph_command_id.hpp│   ├── playback_graph_revision.hpp│   ├── playback_graph_capability.hpp│   └── playback_graph_evidence.hpp│├── policy/│   ├── playback_graph_policy.hpp│   ├── playback_graph_policy_preset.hpp│   ├── graph_tuning_parameters.hpp│   ├── graph_tuning_validation.hpp│   ├── graph_timeout_policy.hpp│   ├── graph_deadline_policy.hpp│   ├── graph_queue_policy.hpp│   ├── queue_budget_dimension_policy.hpp│   ├── graph_memory_budget.hpp│   ├── graph_performance_budget.hpp│   ├── graph_exception_safety_policy.hpp│   ├── graph_abi_policy.hpp│   ├── graph_logging_policy.hpp│   ├── graph_recovery_policy.hpp│   └── graph_av_sync_policy.hpp│├── session/│   ├── playback_session_contract.hpp│   ├── playback_session_snapshot.hpp│   ├── playback_session_options.hpp│   ├── playback_session_async_result.hpp│   ├── playback_session_seek_request.hpp│   ├── playback_session_close_policy.hpp│   └── playback_session_system_event.hpp│├── p4_bridge/│   ├── p4_state_sync_adapter.hpp│   ├── p7_command_intent_sidecar.hpp│   └── p4_revision_bridge.hpp│├── graph/│   ├── graph_orchestrator.hpp│   ├── graph_operation_context.hpp│   ├── graph_operation_coordinator.hpp│   ├── playback_graph_builder.hpp│   ├── playback_graph_topology.hpp│   ├── playback_graph_node.hpp│   ├── playback_graph_edge.hpp│   └── rebuild_granularity_matrix.hpp│├── binding/│   ├── attachment_async_token.hpp│   ├── attachment_completion_event.hpp│   ├── attachment_completion_timer.hpp│   ├── attachment_operation_registry.hpp│   ├── attachment_operation_tombstone.hpp│   ├── av_pipeline_binding.hpp│   ├── source_pipeline_attachment.hpp│   ├── demux_pipeline_attachment.hpp│   ├── audio_pipeline_attachment.hpp│   ├── video_pipeline_attachment.hpp│   ├── av_sync_decision_delivery.hpp│   └── pipeline_attachment_result.hpp│├── command/│   ├── graph_command_router.hpp│   ├── graph_command_queue.hpp│   ├── pending_graph_command_queue.hpp│   ├── pending_command_replay_policy.hpp│   ├── pending_command_replay_result.hpp│   ├── command_lifetime_registry.hpp│   ├── graph_command_priority.hpp│   ├── graph_command_conflict_matrix.hpp│   ├── graph_command_conflict_matrix_source.hpp│   ├── graph_command_coalescer.hpp│   ├── graph_command_barrier.hpp│   ├── graph_command_result.hpp│   ├── graph_command_sequence.hpp│   └── graph_command_cancellation.hpp│├── coordination/│   ├── phase_bitset.hpp│   ├── phase_timeout_policy.hpp│   ├── seek_flush_coordinator.hpp│   ├── drain_cancel_coordinator.hpp│   ├── pending_command_replay_coordinator.hpp│   ├── pause_resume_coordinator.hpp│   ├── track_switch_coordinator.hpp│   ├── buffering_coordinator.hpp│   ├── rebuffering_coordinator.hpp│   ├── stalled_coordinator.hpp│   ├── eos_drain_coordinator.hpp│   └── close_coordinator.hpp│├── clock/│   ├── graph_clock_master_policy.hpp│   ├── av_sync_decision.hpp│   ├── av_sync_decision_result.hpp│   ├── graph_clock_snapshot.hpp│   ├── graph_clock_epoch.hpp│   ├── graph_clock_drift_policy.hpp│   └── graph_startup_preroll_policy.hpp│├── recovery/│   ├── graph_recovery_coordinator.hpp│   ├── graph_recovery_reason.hpp│   ├── graph_recovery_reason_mapping.hpp│   ├── graph_recovery_action.hpp│   ├── graph_recovery_domain.hpp│   ├── graph_recovery_escalation_level.hpp│   ├── combined_domain_failure.hpp│   ├── secondary_domain_verifier.hpp│   ├── source_resume_verifier.hpp│   ├── graph_recovery_fuse.hpp│   └── graph_recovery_snapshot.hpp│├── resources/│   ├── graph_resource_retire_queue.hpp│   ├── graph_resource_recycle_coordinator.hpp│   ├── graph_resource_recycle_barrier.hpp│   ├── graph_recycle_barrier_state.hpp│   ├── graph_frame_lease.hpp│   └── graph_packet_lease.hpp│├── evidence/│   ├── graph_evidence_collector.hpp│   ├── graph_evidence_record.hpp│   ├── graph_evidence_sampling_policy.hpp│   ├── graph_evidence_retention_policy.hpp│   ├── graph_correlation_id.hpp│   ├── graph_evidence_reason.hpp│   ├── graph_evidence_causality.hpp│   ├── graph_evidence_privacy.hpp│   └── graph_evidence_summary.hpp│├── metrics/│   ├── graph_metrics_collector.hpp│   ├── graph_metrics_sampling_policy.hpp│   ├── graph_phase_latency.hpp│   └── graph_metrics_snapshot.hpp│├── events/│   ├── graph_event_sink.hpp│   ├── graph_event.hpp│   ├── graph_event_queue.hpp│   ├── graph_event_priority.hpp│   ├── critical_event_ring.hpp│   ├── graph_snapshot_delta.hpp│   └── graph_observer_contract.hpp│├── health/│   ├── graph_health.hpp│   └── graph_health_snapshot.hpp│├── subtitle_slot/│   ├── subtitle_reserved_node.hpp│   ├── subtitle_sync_anchor.hpp│   ├── subtitle_timeline_stamp.hpp│   └── subtitle_generation_slot.hpp│└── gaps/    ├── contract_gap_report.hpp    ├── contract_gap_registry.hpp    └── missing_carryover_report.hpp

7.2 Private runtime family

backend/src/video/playback_graph/│├── contracts/├── policy/├── session/├── p4_bridge/├── graph/├── binding/├── command/├── coordination/├── clock/├── recovery/├── resources/├── evidence/├── metrics/├── events/├── health/├── subtitle_slot/└── gaps/

7.3 Private test support family

所有 fake / deterministic scheduler / harness 必须放私有测试路径：

backend/tests/video/playback_graph/test_support/│├── fake_end_to_end_playback_harness.hpp├── deterministic_fake_scheduler.hpp├── fake_clock.hpp├── fake_manual_event_pump.hpp├── fake_timeout_injector.hpp├── fake_starvation_injector.hpp├── fake_late_callback_injector.hpp├── fake_recycle_ack_injector.hpp├── fake_queue_overflow_injector.hpp├── fake_generation_stamp_publisher.hpp├── fake_seqlock.hpp├── fake_attachment_async_token.hpp├── fake_phase_bitset.hpp├── fake_critical_event_ring.hpp├── fake_source_session.hpp├── fake_demux_attachment.hpp├── fake_audio_pipeline.hpp├── fake_video_pipeline.hpp└── fake_evidence_sink.hpp

禁止：

include/kivo/video/playback_graph/test_contract/**include/kivo/video/playback_graph/fake/**include/kivo/video/playback_graph/test_support/**

7.4 文件职责与行数

普通文件软限制：

public header soft limit: 400 linescpp soft limit: 500 linescore coordinator soft limit: 800 linescore coordinator hard limit: 1000 lines

规则：

### 1. 单一职责优先于机械行数。2. public header 可以通过主题子结构体拆分，不允许 _part1/_part2。3. 超过 soft limit 必须在 Final Report 解释原因。4. 超过 hard limit 必须拆分。5. 禁止 misc/common/utils/helpers/manager/all_in_one。

### 8. Policy / Tuning / Safety Validation

8.1 PlaybackGraphPolicy 必须包含

timeout_policydeadline_policyqueue_policypending_command_policymemory_budget_policyperformance_budget_policyexception_safety_policyabi_policylogging_policybuild_retry_policypre_start_seek_policypre_start_switch_policyseek_coalescing_policyeos_seek_policyplay_after_eos_policybuffering_policyrebuffering_policypause_intent_policystalled_policyclock_master_policyav_sync_policyclock_drift_policyaudio_silence_fill_policymicro_adjust_policycapability_fallback_policyprotected_content_policyrecovery_policycombined_domain_failure_policysecondary_domain_verification_policysource_resume_policyresource_recycle_policyresource_retire_policyobserver_delivery_policyevidence_privacy_policyevidence_sampling_policyevidence_retention_policymetrics_sampling_policylive_policygraph_health_command_policy

8.2 GraphTuningParameters 必须包含

command_queue_capacitypending_command_queue_capacitypending_command_max_age_mspipeline_event_queue_capacitycritical_event_slot_capacitycritical_event_slot_version_bitsseqlock_max_retry_countevidence_queue_capacitymetrics_queue_capacityobserver_queue_capacityaudio_packet_queue_budget_bytesaudio_packet_queue_budget_duration_msvideo_frame_queue_budget_bytesvideo_frame_queue_budget_framesvideo_frame_queue_budget_duration_msqueue_budget_modequeue_budget_dimension_policyinitial_buffering_min_audio_msinitial_buffering_min_video_framesrebuffering_threshold_msstalled_threshold_msstalled_sample_interval_msstalled_sample_jitter_tolerance_msstalled_debounce_window_msstalled_required_consecutive_samplesseek_collapse_window_mssource_seek_timeout_msdemux_seek_timeout_msaudio_flush_timeout_msvideo_flush_timeout_msseek_timeout_msphysical_retire_enqueue_timeout_msphysical_recycle_timeout_mstrack_switch_timeout_msrecovery_timeout_mssecondary_verification_timeout_mssource_resume_verify_timeout_msclose_timeout_msattachment_completion_timeout_msmax_build_retry_countmax_same_domain_recovery_countmax_same_reason_recovery_countmax_total_recovery_countrecovery_fuse_window_msmax_lease_recycle_retry_countmax_recycle_barrier_retry_countmax_background_retired_resource_countclock_drift_warning_threshold_msclock_drift_fatal_threshold_msdefault_audio_micro_adjust_min_ratiodefault_audio_micro_adjust_max_ratiocapability_audio_micro_adjust_min_ratiocapability_audio_micro_adjust_max_ratiocommand_accept_budget_mssnapshot_read_budget_ushot_path_event_submit_budget_usseek_to_first_frame_budget_mspause_command_to_paused_budget_msresume_to_playing_budget_mstrack_switch_budget_msrecovery_budget_msclose_budget_msav_sync_decision_budget_us

Runtime counters 不属于 tuning：

current_build_retry_countcurrent_recovery_countcurrent_lease_retry_countcurrent_pending_age_ms

这些必须存在于 runtime state / operation context，不得写入 static tuning。

8.3 Seqlock retry validation

seqlock_max_retry_count 必须固定默认值 3。允许 debug/test 降低到 0/1 以验证 fallback。生产不得高于 3。

Validation：

seqlock_max_retry_count < 0 -> rejectseqlock_max_retry_count > 3 -> reject

8.4 Safety-critical validation rules

以下必须 reject，禁止 clamp：

close_timeout_ms <= 0physical_retire_enqueue_timeout_ms <= 0physical_recycle_timeout_ms <= 0recovery_timeout_ms <= 0secondary_verification_timeout_ms <= 0source_resume_verify_timeout_ms <= 0attachment_completion_timeout_ms <= 0max_total_recovery_count <= 0max_same_domain_recovery_count <= 0max_same_reason_recovery_count <= 0recovery_fuse_window_ms <= 0max_lease_recycle_retry_count <= 0max_recycle_barrier_retry_count <= 0max_background_retired_resource_count <= 0command_queue_capacity == 0pending_command_queue_capacity == 0pipeline_event_queue_capacity == 0critical_event_slot_capacity == 0critical_event_slot_version_bits < 16observer_queue_capacity == 0stalled_sample_interval_ms <= 0stalled_sample_jitter_tolerance_ms < 0stalled_debounce_window_ms <= stalled_required_consecutive_samples * stalled_sample_interval_ms + stalled_sample_jitter_tolerance_msstalled_required_consecutive_samples <= 0seek_collapse_window_ms <= 0source_seek_timeout_ms <= 0demux_seek_timeout_ms <= 0audio_flush_timeout_ms <= 0video_flush_timeout_ms <= 0seek_timeout_ms <= 0pending_command_max_age_ms <= 0clock_drift_warning_threshold_ms <= 0clock_drift_fatal_threshold_ms <= clock_drift_warning_threshold_msdefault_audio_micro_adjust_min_ratio <= 0default_audio_micro_adjust_max_ratio <= 0default_audio_micro_adjust_min_ratio > 1.0default_audio_micro_adjust_max_ratio < 1.0default_audio_micro_adjust_min_ratio >= default_audio_micro_adjust_max_ratioprotected_content_policy allows unsafe fallbackgraph_total_memory_budget_bytes < required_minimum_queue_budget

8.5 Queue budget explicit dimension policy

V1.9 修正 V1.8 的“raw zero 半配置”问题。

禁止用裸 0 表示“忽略某个维度”。

必须使用显式维度开关：

enum class QueueBudgetDimensionMode : std::uint32_t {    Disabled = 0,    Bounded = 1};struct VideoQueueBudgetPolicy {    QueueBudgetDimensionMode frames_mode;    std::uint32_t frames_limit;    QueueBudgetDimensionMode bytes_mode;    std::uint64_t bytes_limit;    QueueBudgetDimensionMode duration_mode;    std::uint32_t duration_ms_limit;};struct AudioQueueBudgetPolicy {    QueueBudgetDimensionMode bytes_mode;    std::uint64_t bytes_limit;    QueueBudgetDimensionMode duration_mode;    std::uint32_t duration_ms_limit;};

规则：

1. Bounded 模式下对应 limit 必须 > 0。2. Disabled 模式下对应 limit 必须 == 0。3. 至少一个维度必须 Bounded。4. 全部 Disabled 只允许 QueueBudgetMode::UnlimitedForTestOnly。5. 生产默认禁止 UnlimitedForTestOnly。6. 半配置允许，但必须通过显式 DimensionMode 表达，不允许 raw zero 产生歧义。

8.6 Micro-adjust validation

默认安全范围：

default_audio_micro_adjust_min_ratio >= 0.999default_audio_micro_adjust_max_ratio <= 1.001

能力扩展范围：

capability_audio_micro_adjust_min_ratio >= P6Capability.min_ratiocapability_audio_micro_adjust_max_ratio <= P6Capability.max_ratio

联合判定：

1. 如果 policy 使用 default mode，则必须落在 0.999 - 1.001。2. 如果 policy 使用 capability-extended mode，则必须先确认 P6 capability 支持。3. capability-extended mode 下允许超过 0.999 - 1.001，但不得超过 P6 capability。4. 如果 P6 支持 0.990 - 1.010，policy 可以选择更大范围，但必须被 capability 明确授权。5. 如果 P6 不支持 dynamic micro-adjust，则 policy 必须禁用该路径或返回 typed unsupported。

Capability 查询时机：

1. Graph build 时查询 P6 MicroAdjustCapability。2. Audio track switch 后重新查询。3. Audio device change 后重新查询。4. SystemResume 后重新查询。5. Capability 变化后必须刷新 cached policy decision。6. 当前 policy 超出新 capability 时，必须降级到 default mode 或禁用 micro-adjust，并 emit MicroAdjustCapabilityChanged evidence。

8.7 PolicyPreset

必须提供：

LocalFilePresetNasLanPresetVodStreamingPresetLiveStreamingPresetLowLatencyLivePresetDebugDeterministicPreset

DebugDeterministicPreset：

### 1. 禁止 wall-clock sleep。2. 所有 timestamp 由 FakeClock 提供。3. invalid tuning 必须 reject，不允许 clamp。4. evidence full mode。

### 9. Hot Path Contract

9.1 Hot Path 线程集合

线程

是否 Hot Path

说明

AudioOutputThread

是

直接输出音频到设备

VideoPresentationThread

是

直接呈现视频帧

P5 高频 render/present callback

是

与 D3D/present 节奏相关

P6 高频 audio/device callback

是

与 audio device period 相关

GraphOwnerThread

否

状态管理、命令分发、phase 推进

EvidenceBuildThread

否

证据构建

MetricsBuildThread

否

指标收集

ObserverDeliveryThread

否

对外 event/snapshot delta

RecoveryCoordinator execution context

否

恢复协调，不是实时输出

9.2 Hot Path 禁止操作

操作

禁止

替代方案

dynamic allocation

是

preallocated pool / fixed slot

mutex lock

是

lock-free / atomic / precomputed snapshot

future wait

是

event token

blocking queue push

是

bounded nonblocking offer

file IO

是

evidence sink later

network IO

是

async source runtime

logging / formatting

是

compact preallocated event

QString/std::string formatting

是

fixed buffer / ids

sleep

是

drop / yield only if explicitly bounded

unbounded retry loop

是

max retry count

graph mutable state access

是

published stamp/snapshot only

9.3 Hot Path 验证机制

Static guard：

1. path-scoped token scan。2. hot path source files 禁止 new/malloc/free/delete。3. 禁止 std::vector::push_back / std::string mutation / std::ostringstream。4. 禁止 LOG / printf / sprintf / QString / QFile / std::ofstream。5. 禁止 std::mutex / std::unique_lock / std::condition_variable。6. 禁止 std::future::get / wait。

Runtime debug guard：

1. Hot Path 线程启动时注册 thread role。2. Debug allocator 检测 hot path allocation。3. Hot path logging API 调用触发 assert。4. Hot path blocking API 调用触发 assert。5. Hot path retry 超限必须 emit evidence，不得继续 retry。

### 10. P4 Bridge / Command Intent

10.1 P7↔P4 状态同步唯一方案

V1.9 锁定唯一方案：

P7 GraphOwnerThread→ P4StateSyncAdapter::apply_graph_transition(...)→ P4 validates PlaybackCommandId / PlaybackRevision / transition intent / generation stamp consistency→ P4 returns StateTransitionResult + PlaybackRevision→ P7 stores returned PlaybackRevision in immutable snapshot

10.2 PlaybackRevision 所有权

PlaybackRevision 是 P4 的逻辑版本号。PlaybackRevision 只由 P4 分配。P7 不得生成、递增、伪造、重置 PlaybackRevision。P7 只能携带 expected_revision，并接受 P4 返回的 revision_after。

10.3 P4StateSyncAdapter 是纯语义校验器

P4StateSyncAdapter 只负责：

### 1. 验证 P4 command/revision/state transition 语义。2. 验证 generation_stamp 与 expected_revision 的逻辑一致性。3. 返回 P4 分配的 PlaybackRevision。4. 返回 P4 PlaybackState。5. 返回 typed failure。

P4StateSyncAdapter 不负责：

### 1. 查询 P5/P6 queue。2. 查询 source 状态。3. 查询真实 audio/video clock。4. 等待 attachment。5. 判断底层设备是否真实 ready。6. 做 IO。7. 做 blocking wait。8. 分配 P7 资源。

底层物理状态验证必须由 P7 在调用 apply_graph_transition() 之前完成：

AttachmentCompletionEventQueueSnapshotSecondaryDomainVerifierSourceResumeVerifierGraphClockCoordinator

10.4 P4StateSyncAdapter 接口

struct P4GraphTransitionInput {    PlaybackCommandId command_id;    GraphCorrelationId correlation_id;    PlaybackGraphState previous_graph_state;    PlaybackGraphState next_graph_state;    TimelinePosition timeline_position;    PlaybackGraphError error;    GraphTransitionReason reason;    PlaybackRevision expected_revision;    GenerationStamp generation_stamp;};struct P4GraphTransitionResult {    bool accepted;    PlaybackRevision revision_after;    PlaybackState p4_state_after;    PlaybackGraphError error;};class P4StateSyncAdapter {public:    Result<P4GraphTransitionResult, PlaybackGraphError> apply_graph_transition(        const P4GraphTransitionInput& input) noexcept;};

错误：

GraphRevisionMismatch:  P4 检测 expected_revision 与 generation_stamp 对应的逻辑 revision 不一致。P4StateSyncFailed:  P4 拒绝合法性不满足的 transition。GraphInvariantBroken:  P7 状态、generation、revision 出现无法调和的不变量破坏。

10.5 P7CommandIntentSidecar

P7 不修改 P4 frozen command header。

Seek / EOS / preview / replay intent 使用 sidecar：

P7CommandIntentSidecar  key: PlaybackCommandId  value:    command_intent_kind    seek_kind: PreviewSeek / CommitSeek / ProgrammaticSeek / EosSeek    play_after_seek: bool    collapse_group_id    target_timeline    created_timestamp

规则：

1. 外部命令仍先创建 P4 PlaybackCommand。2. P7 以 PlaybackCommandId 绑定 sidecar intent。3. P4 不需要理解 PreviewSeek / CommitSeek 细节。4. P4 仍负责 command identity、revision、基础 state validation。5. Sidecar 生命周期必须覆盖 command accepted -> completed/cancelled/failed。6. command 结束后 sidecar 必须由 GraphOwnerThread 清理。7. 如果 P4 无稳定 PlaybackCommandId，触发 STOP。

### 11. PlaybackSession API / AsyncResult

11.1 Public API

struct SeekRequest {    TimelinePosition target;    SeekKind kind;                 // PreviewSeek / CommitSeek / ProgrammaticSeek / EosSeek    bool play_after_seek;    CollapseGroupId collapse_group_id;};class PlaybackSession {public:    AsyncResult open(const OpenRequest& request) noexcept;    AsyncResult start() noexcept;    AsyncResult pause() noexcept;    AsyncResult resume() noexcept;    AsyncResult seek(const SeekRequest& request) noexcept;    AsyncResult switch_audio_track(const TrackSwitchRequest& request) noexcept;    AsyncResult switch_video_track(const TrackSwitchRequest& request) noexcept;    AsyncResult close() noexcept;    PlaybackSessionSnapshot snapshot() const noexcept;    SubscriptionResult subscribe_events(GraphObserver& observer) noexcept;    AsyncResult inject_system_event(const SystemEvent& event) noexcept;};

11.2 AsyncResult 非阻塞定义

AsyncResult 不是 future。

AsyncResult 必须是非阻塞 command token：

struct AsyncResult {    PlaybackCommandId command_id;    GraphCorrelationId correlation_id;    AsyncResultState state;    PlaybackGraphError immediate_error;    SnapshotRevision snapshot_revision_at_accept;};

禁止：

AsyncResult::get()AsyncResult::wait()AsyncResult::blocking_result()std::future-like behavior

完成状态只能通过：

### 1. GraphEventSink / observer event。2. CommandCompleted event。3. CommandFailed event。4. snapshot polling。

11.3 AsyncResult 生命周期

创建时机：

PlaybackSession::open/start/pause/resume/seek/switch/close 接收命令并获得 PlaybackCommandId 后创建。

所有权：

AsyncResult record 由 PlaybackSession 拥有。CommandLifetimeRegistry 只能由 GraphOwnerThread 修改。hot path 不访问 CommandLifetimeRegistry。

销毁时机：

### 1. CommandCompleted。2. CommandFailed。3. CommandCancelled。4. CancelledByClose。5. PlaybackSession destruction。

late event 规则：

### 1. Completion event 到达时如果 command_id 已不存在，忽略该 event。2. 必须 emit LateCommandCompletionDiscarded evidence。3. late event 不得复活 command。4. late event 不得访问已销毁 AsyncResult record。

11.4 AsyncResult / Sidecar / CommandId 绑定

1. PlaybackCommandId 由 P4 分配。2. AsyncResult 与 PlaybackCommandId 一一对应。3. P7CommandIntentSidecar 以 PlaybackCommandId 为 key。4. PendingCommand 以 PlaybackCommandId + GraphCorrelationId 为 identity。5. Attachment operation 可以引用 command_id，但不能拥有 AsyncResult。6. Hot path event 只携带 command_id/correlation_id/stamp，不访问 command map。

11.5 Internal-only API

drain_internal()recover_internal()cancel_drain_internal()

不得暴露给 P9/UI。

11.6 Idempotent command policy

固定规则：

resume while Playing -> NoOppause while Paused -> NoOppause while EosReached -> NoOpclose while Closing -> NoOp or last close resultclose while Closed -> NoOp or last close resultsame track switch -> NoOpstart while Playing -> InvalidStatestart while Paused -> InvalidStateresume while EosReached -> EosReachedRequiresSeek

ClosedGraph 只用于：

Closed 状态下收到非 close / snapshot 的 mutating command。

close 本身在 Closed 下不是 ClosedGraph 错误。

### 12. PlaybackGraph State Machine

12.1 State enum

NotCreatedCreatedBuildingBuildFailedReadyStartingBufferingPlayingRebufferingStalledPausingPausedResumingSeekingFlushingTrackSwitchingDrainingEosReachedSystemSuspendingSystemSuspendedSystemResumingRecoveringFailedClosingClosed

12.2 State groups

NonTerminalState：

NotCreatedCreatedBuildingBuildFailedReadyStartingBufferingPlayingRebufferingStalledPausingPausedResumingSeekingFlushingTrackSwitchingDrainingEosReachedSystemSuspendingSystemSuspendedSystemResumingRecoveringFailedClosing

TerminalState：

Closed

12.3 Closed transition rule

硬规则：

除 Closed 自身外，所有进入 Closed 的路径必须经过 Closing。禁止 BuildFailed -> Closed。禁止 Failed -> Closed。禁止 EosReached -> Closed。禁止 NotCreated -> Closed 直接跳转。

唯一合法终结路径：

AnyNonTerminalState -> Closing -> Closed

12.4 Fast Closing Path

适用状态：

NotCreatedCreatedBuildFailedFailed

Fast Closing Path 操作：

1. mark Closing。2. cancel pending commands。3. invalidate command registry entries。4. invalidate sidecar entries。5. if Failed: emit GraphFailed evidence before close evidence。6. if Failed: skip flush/recycle if attachments are already invalid or unsafe to touch。7. if NotCreated/Created/BuildFailed: skip flush/recycle/detach because no pipeline resource is owned。8. publish final snapshot with Closing then Closed。9. emit GraphClosedFromClean or GraphClosedFromFailed evidence。10. enter Closed。

Fast Closing Path 禁止：

### 1. 直接跳 Closed。2. 访问已 invalid 的 P5/P6 attachment。3. 强行 flush 已 fatal 的 pipeline。4. double free。

12.5 合法状态转移

NotCreated -> CreatedNotCreated -> ClosingCreated -> BuildingCreated -> ClosingBuilding -> ReadyBuilding -> BuildFailedBuilding -> RecoveringBuilding -> FailedBuilding -> ClosingBuildFailed -> Building only if build_retry_policy == enabled and retry budget remainsBuildFailed -> ClosingReady -> StartingReady -> Seeking only if pre_start_seek_policy.enabled == trueReady -> TrackSwitching only if pre_start_switch_policy.enabled == trueReady -> ClosingReady -> FailedStarting -> BufferingStarting -> PlayingStarting -> RecoveringStarting -> FailedStarting -> ClosingBuffering -> PlayingBuffering -> RebufferingBuffering -> RecoveringBuffering -> FailedBuffering -> ClosingBuffering -> SystemSuspendingPlaying -> RebufferingPlaying -> PausingPlaying -> SeekingPlaying -> TrackSwitchingPlaying -> DrainingPlaying -> RecoveringPlaying -> FailedPlaying -> ClosingPlaying -> SystemSuspendingRebuffering -> PlayingRebuffering -> Pausing if pause_intent_pending == trueRebuffering -> StalledRebuffering -> RecoveringRebuffering -> FailedRebuffering -> ClosingRebuffering -> SystemSuspendingStalled -> Rebuffering if data returns before recovery selectedStalled -> Seeking if seek command acceptedStalled -> Recovering when stalled policy selects recoveryStalled -> FailedStalled -> ClosingStalled -> SystemSuspendingPausing -> PausedPausing -> RecoveringPausing -> FailedPausing -> ClosingPausing -> SystemSuspendingPaused -> ResumingPaused -> SeekingPaused -> TrackSwitchingPaused -> RecoveringPaused -> FailedPaused -> ClosingPaused -> SystemSuspendingResuming -> PlayingResuming -> RebufferingResuming -> RecoveringResuming -> FailedResuming -> ClosingResuming -> SystemSuspendingSeeking -> FlushingSeeking -> RecoveringSeeking -> FailedSeeking -> ClosingSeeking -> SystemSuspendingFlushing -> BufferingFlushing -> PlayingFlushing -> PausedFlushing -> RecoveringFlushing -> FailedFlushing -> ClosingFlushing -> SystemSuspendingTrackSwitching -> BufferingTrackSwitching -> PlayingTrackSwitching -> PausedTrackSwitching -> RecoveringTrackSwitching -> FailedTrackSwitching -> ClosingTrackSwitching -> SystemSuspendingDraining -> EosReachedDraining -> RecoveringDraining -> FailedDraining -> ClosingDraining -> Seeking only through drain_cancel_by_seekDraining -> SystemSuspendingEosReached -> SeekingEosReached -> RecoveringEosReached -> FailedEosReached -> ClosingSystemSuspending -> SystemSuspendedSystemSuspending -> ClosingSystemSuspended -> SystemResumingSystemSuspended -> ClosingSystemResuming -> Recovering if source/audio/video/display/HDR requires recoverySystemResuming -> BufferingSystemResuming -> PlayingSystemResuming -> PausedSystemResuming -> FailedSystemResuming -> ClosingRecovering -> BufferingRecovering -> PlayingRecovering -> PausedRecovering -> Ready only if recovery rebuilt graph but did not restart pipelinesRecovering -> FailedRecovering -> ClosingFailed -> ClosingClosing -> Closed

12.6 Rebuffering pause intent

Rebuffering 状态收到 pause：

1. 设置 pause_intent_pending。2. 保存 command_id/correlation_id。3. 如果 output 可以立即 hold，则 Rebuffering -> Pausing -> Paused。4. 如果必须等待 rebuffer phase 完成，则 Rebuffering 结束后不得回 Playing，必须进入 Pausing -> Paused。5. 禁止 Rebuffering -> Paused 直接跳过 Pausing。

can_hold_immediately 判定：

Video side:  P5 reports hold_supported == true  and RendererQueueEmpty == true  and PresentQueueCommitted == true  or video pipeline is not selected.Audio side:  P6 reports hold_supported == true  and DeviceBufferDrained == true  and OutputDrained == true  or audio pipeline is not selected.

如果无法同步 bounded 查询 hold readiness：

### 1. 保持 pause_intent_pending。2. 等待 Rebuffering 完成或等待 attachment event。3. 不阻塞 GraphOwnerThread。

Pause intent 超时：

当前状态

超时后行为

目标状态

Rebuffering

expire pause intent, emit PendingCommandExpired

Rebuffering

Pausing

complete best-effort pause, emit PendingCommandExpired

Paused

其他

ignore expired intent as superseded

current state

12.7 EosReached 命令语义

pause -> NoOpresume -> EosReachedRequiresSeekstart/play ->  play_after_eos_policy == RestartFromBeginning: 转换为 seek_to_start_and_play  play_after_eos_policy == Reject: EosReachedRequiresSeekseek -> accepted if target validclose -> acceptedsnapshot -> immediate

### 13. Command Conflict Matrix

13.1 Source-of-truth

命令冲突矩阵必须代码化，不能只有文档表格。

必须存在：

graph_command_conflict_matrix_source.hpp/cpp

或等价数据文件，由代码生成：

### 1. runtime conflict resolver。2. full matrix unit tests。3. documentation table。

禁止：

文档一套规则，代码一套规则。

13.2 Full Command Conflict Matrix

State

close

fatal recover

seek

pause

resume

track switch

drain internal

snapshot

NotCreated

enter Closing

reject

reject

reject

reject

reject

reject

empty snapshot

Created

enter Closing

reject

reject

reject

reject

reject

reject

immediate

Building

close wins

recover if transient build failure

reject/pending by policy

reject

reject

reject

reject

immediate

BuildFailed

close wins

retry/recover by build_retry_policy

reject

reject

reject

reject

reject

immediate

Ready

close wins

recover if attachment/source failure

accept only if pre_start_seek_policy.enabled

NoOp/reject by policy

reject

accept only if pre_start_switch_policy.enabled

reject

immediate

Starting

close wins

recover wins

pending/collapse

pending pause intent

reject

pending/reject

reject

immediate

Buffering

close wins

recover wins

accept/collapse

accept pause intent

pending/reject

pending/reject

reject

immediate

Playing

close wins

recover wins

accept/collapse

accept

NoOp

accept

internal only

immediate

Rebuffering

close wins

recover wins

accept/collapse

accept pause intent

pending/reject

pending/reject

reject

immediate

Stalled

close wins

recover wins

accept/collapse

accept pause intent

reject/pending by policy

pending/reject

reject

immediate

Pausing

close wins

recover if fatal

pending/collapse

already pausing

pending resume intent by policy

pending/reject

reject

immediate

Paused

close wins

recover wins

accept/collapse

NoOp

accept

accept

reject

immediate

Resuming

close wins

recover if fatal

pending/collapse

pending pause intent

already resuming

pending/reject

reject

immediate

Seeking

close wins

recover if fatal

collapse latest

pending post-seek pause

pending/reject

pending/reject

reject

immediate

Flushing

close wins

recover if fatal

collapse latest or cancel current seek by policy

pending post-flush pause

pending/reject

pending/reject

reject

immediate

TrackSwitching

close wins

recover if fatal

pending/collapse

pending

pending/reject

replace if different track; NoOp if same track

reject

immediate

Draining

close wins

recover if fatal

cancel drain + seek

reject/pending

reject

reject

already draining

immediate

EosReached

close wins

recover if device/source failure

accept EOS seek

NoOp

EosReachedRequiresSeek

reject

reject

immediate

SystemSuspending

close wins

reject unless fatal

pending

pending

reject

pending/reject

reject

immediate

SystemSuspended

close wins

reject unless fatal

pending/reject by policy

NoOp

pending system resume

pending/reject

reject

immediate

SystemResuming

close wins

recover if resume failure

pending/collapse

pending

pending

pending/reject

reject

immediate

Recovering

close wins

already recovering

pending-latest

pending/reject

pending/reject

pending/reject

reject

immediate

Failed

close wins

reject unless privileged repair policy

reject

reject

reject

reject

reject

immediate

Closing

NoOp / last close result

reject

cancel

cancel

cancel

cancel

cancel

last snapshot

Closed

NoOp / last close result

ClosedGraph

ClosedGraph

ClosedGraph

ClosedGraph

ClosedGraph

ClosedGraph

last snapshot

13.3 Matrix validation

必须自动验证：

1. 所有 PlaybackGraphState 都有行。2. 所有 public command 都有列。3. Closed close 是 NoOp/last close result。4. Closed 非 close/snapshot mutating command 是 ClosedGraph。5. Closing 不接受新 mutating command。6. close wins in all non-terminal states。7. 无未定义 cell。8. 无重复 state/command entry。

测试必须 100% 覆盖 state × command 组合，并加入 deterministic fuzz：

random command sequencerandom injected failuresrandom pending replay orderrandom close preemption

### 14. Pending Command Queue / Replay

14.1 Pending queue parameters

pending_command_queue_capacitypending_command_max_age_ms

Pending 类型：

PendingLatestSeekPendingPauseIntentPendingResumeIntentPendingTrackSwitchPendingHealthRestrictedCommandPendingSystemResumeCommand

14.2 Pending rules

1. seek pending-latest：只保留最新 target。2. pause/resume：同一类只保留最新 intent。3. track switch：same track -> NoOp；same kind different track -> replace；audio/video combined -> merge。4. close 到来时所有 pending command -> CancelledByClose。5. pending 超时 -> PendingCommandExpired。6. recovery/system resume 完成后必须 replay/reject/expire。

14.3 Recovery pending replay

Recovery 完成后进入 replay phase：

1. Recovery action completed。2. Secondary domains verified or marked degraded。3. Determine restored_state。4. Drop expired pending commands。5. Collapse seek to latest。6. Revalidate target against new SourceGeneration/DemuxGeneration/timeline range。7. Replay order:   latest seek   pause intent   resume intent   track switch8. Re-enqueue accepted pending commands into GraphOwnerThread command loop。9. Emit PendingCommandReplayed / PendingCommandRejected / PendingCommandExpired evidence。10. Transition Recovering -> restored_state or immediate next command state。

14.4 Replay Interruption Policy

如果 pending replay 中任一命令失败：

1. 立即停止后续 pending replay。2. 尚未 replay 的 pending command 返回 PendingCommandReplayAborted。3. 生成 PendingCommandReplayAborted evidence。4. 如果失败命令是 seek 且原因是 target invalid after recovery：   graph 进入 Paused，如果 Paused 可用；   否则进入 Failed。5. 如果失败命令是 pause：   graph 保持 restored_state，并报告 pause failed。6. 如果失败命令是 resume：   graph 进入 Paused 或 Buffering，由 restored_state 与 policy 决定。7. 如果失败命令是 track switch：   graph 保持原 track 并进入 Paused 或 restored_state。8. 禁止 replay 一个失败后继续盲目执行后续命令。

### 15. Async Attachment Contract

15.1 Async token

所有可能耗时的 P5/P6/source/demux attachment 方法必须非阻塞，返回 token。

enum class AttachmentKind : std::uint32_t {    Source,    Demux,    Audio,    Video};struct OperationKey {    AttachmentKind attachment_kind;    std::uint64_t operation_id;};struct AttachmentAsyncToken {    OperationKey operation_key;    GraphCorrelationId correlation_id;    PlaybackCommandId command_id;    GenerationStamp generation_stamp;    AttachmentOperationKind kind;};

完成通过 event：

struct AttachmentCompletionEvent {    OperationKey operation_key;    GraphCorrelationId correlation_id;    PlaybackCommandId command_id;    AttachmentOperationKind kind;    AttachmentCompletionStatus status;    PlaybackGraphError error;    GenerationStamp generation_stamp;};

禁止：

GraphOwnerThread future.get()GraphOwnerThread wait()GraphOwnerThread 长时间阻塞调用 attachment

15.2 Operation id 唯一性

规则：

1. operation_id 在同一个 AttachmentKind 内必须单调递增且唯一。2. 跨 AttachmentKind 可重复，但 OperationKey 必须唯一。3. Debug 模式下 AttachmentOperationRegistry 必须检测重复分配。4. operation_id wrap 使用 uint64_t，正常生命周期不可达。5. 测试可用小位宽 fake id 验证 wrap policy。

15.3 Cancel semantics

Cancel 是：

best-effortidempotentnon-blocking

规则：

1. cancel 请求不保证底层操作立即停止。2. 多次 cancel 同一个 OperationKey 只有第一次产生状态变化。3. cancel 后仍可能收到 Success/Failed/Cancelled/Timeout completion。4. operation timeout 后必须创建 tombstone。5. late completion 命中 tombstone 时不得重新推进 phase。6. late completion 必须 emit LateAttachmentCompletionDiscarded evidence。

CompletionStatus：

SuccessFailedCancelledTimeoutUnsupportedUnknown

15.4 Completion timeout

AttachmentCompletionTimeout 必须由 GraphOwnerThread controlled timer / FakeClock 驱动。

超时流程：

1. GraphOwnerThread records OperationKey start timestamp。2. timeout 到达前未收到 completion event。3. Mark operation timed out。4. Create AttachmentOperationTombstone。5. Attempt cancel through attachment cancel contract if available。6. Invalidate active operation entry。7. Emit AttachmentCompletionTimeout evidence。8. Enter domain-specific recovery or FailClosed。

15.5 Phase Bitset

每个 async phase 必须有：

struct PhaseBitset {    std::uint64_t required_mask;    std::uint64_t completed_mask;    std::uint64_t failed_mask;    std::uint64_t timed_out_mask;};

规则：

1. required_mask 在 phase 开始时固定。2. completion event 只能设置 completed/failed/timed_out bit。3. 所有 required bits 完成后进入下一 phase。4. 任一 required bit failed/timed_out 后进入 domain recovery。5. phase 之间不得形成 circular dependency。6. phase timeout 由 FakeClock/GraphOwnerThread timer 驱动。7. GraphOwnerThread 不阻塞等待 phase。

必备测试：

test_phase_bitset_all_completetest_phase_bitset_single_timeouttest_phase_bitset_failuretest_phase_bitset_late_completiontest_phase_bitset_concurrent_completion_ordertest_phase_bitset_no_circular_dependency

15.6 Snapshot query contract

以下 snapshot 查询必须 lock-free 或 bounded extremely-short critical section：

get_video_queue_snapshot()get_audio_queue_snapshot()get_video_clock_snapshot()get_audio_clock_snapshot()get_video_attachment_state()get_audio_attachment_state()

要求：

### 1. 不得做 IO。2. 不得等待 decoder/render/output 线程。3. 不得等待 GraphOwnerThread。4. 允许读取原子计数或 immutable snapshot。5. 如果 P5/P6 无法保证 bounded snapshot，必须改为 async QueueSnapshotEvent。

### 16. GenerationStamp / Seqlock

16.1 最终方案

V1.9 锁定唯一方案：

collision-free opaque per-stream stamp id+ immutable GenerationTupleRegistry+ seqlock publication

RCU-style 只作为 immutable snapshot 生命周期管理方式，不作为另一个发布方案。

16.2 禁止 128-bit atomic

禁止：

std::atomic<GenerationStamp>std::atomic<GenerationTuple>std::atomic<__int128>任何依赖 128-bit lock-free atomic 的实现

必须使用：

std::atomic<std::uint64_t> sequencestd::atomic<std::uint64_t> tuple_idstd::atomic<std::uint64_t> stream_stamp_id

必须静态断言：

static_assert(std::atomic<std::uint64_t>::is_always_lock_free);

如果平台不满足：

STOP: GenerationStamp64AtomicNotLockFree

16.3 GenerationStamp 结构

struct GenerationStamp {    std::uint64_t tuple_id;    std::uint64_t stream_stamp_id;};

AudioGenerationTuple：

graph_generationsource_generationdemux_generationseek_epochrecovery_epochaudio_generationclock_epochresource_recycle_epoch

VideoGenerationTuple：

graph_generationsource_generationdemux_generationseek_epochrecovery_epochvideo_generationclock_epochresource_recycle_epoch

规则：

### 1. tuple_id 单调递增。2. stream_stamp_id per-session + per-stream kind 单调递增。3. 禁止 hash。4. hot path 只比较 GenerationStamp。5. hot path 不访问 GenerationTupleRegistry。

16.4 Seqlock publication

Writer：GraphOwnerThread。Reader：AudioOutputThread / VideoPresentationThread / callback.

规则：

1. GraphOwnerThread 写 immutable GenerationTuple。2. release store tuple pointer / tuple_id。3. release store stream_stamp_id。4. release store seqlock even version。5. hot path acquire load seqlock version。6. hot path acquire load tuple_id。7. hot path acquire load stream_stamp_id。8. hot path acquire load seqlock version again。9. version 不一致或 odd -> retry。10. retry 最多 seqlock_max_retry_count，生产默认 3。11. retry 超限 -> drop current unsubmitted frame/packet, emit StampReadUnstable compact event。12. hot path 不拿 mutex。

16.5 Seqlock retry 伪代码

StampReadResult read_current_stamp_hot_path() noexcept {    for (std::uint32_t retry = 0; retry <= seqlock_max_retry_count; ++retry) {        auto seq1 = sequence.load(std::memory_order_acquire);        if ((seq1 & 1u) != 0u) {            continue;        }        auto tuple = tuple_id.load(std::memory_order_acquire);        auto stamp = stream_stamp_id.load(std::memory_order_acquire);        auto seq2 = sequence.load(std::memory_order_acquire);        if (seq1 == seq2 && ((seq2 & 1u) == 0u)) {            return StampReadResult::ok(GenerationStamp{tuple, stamp}, retry);        }    }    return StampReadResult::unstable(seqlock_max_retry_count);}

16.6 Seqlock 失败丢弃范围

场景

P7 是否丢弃

说明

Video frame 已从 queue 弹出但未提交给 renderer

是

P7/P5 boundary 前，可安全丢弃

Video frame 已提交给 renderer 但未 present

否

由 P5 内部 generation check / discard

Video frame 已 present

否

已不可回滚，只能 evidence

Audio packet 已从 queue 弹出但未写入 device buffer

是

P7/P6 boundary 前，可安全丢弃或 silence fill

Audio packet 已写入 device buffer

否

由 P6 内部 generation check / silence policy

Audio packet 已输出到设备

否

已不可回滚，只能 evidence

硬规则：

Seqlock 读取失败时，P7 不得继续提交当前未提交 frame/packet。已提交给 P5/P6 的数据由 P5/P6 自己做 generation check。

16.7 Slow path evidence expansion

Compact stale event 必须携带：

observed_stampexpected_stamp_snapshot_idmedia_kindframe_or_packet_idlease_id if resource-backedretry_counttimestamp_mono

Slow path：

### 1. EvidenceBuildThread 读取 observed_stamp.tuple_id。2. 读取 expected_stamp_snapshot_id。3. 展开 mismatch 字段。4. 生成完整 StaleDiscard evidence。5. tuple 已清理时生成 StaleDiscardEvidencePartial。

16.8 Wrap 量化说明

按最极端假设：

每 1 微秒递增一次 uint64 stamp，2^64 微秒约等于 584,000 年。

CriticalEvent sequence 同理使用 uint64_t。每秒 100 万次 event，约 584,000 年 wrap。正常产品生命周期视为不可达。

测试使用小位宽 fake generation/sequence 验证 wrap policy。

### 17. Critical Event Contract

17.1 Critical event slot 禁止 ABA

禁止使用：

exchange(0)clear all flagsreset whole critical flag word

必须使用以下之一：

### 1. fetch_and(~consumed_mask, memory_order_acq_rel)，只清除已消费 bit。2. VersionedCriticalEventSlot，每个 slot 包含 sequence/version。3. CriticalEventRing，每个 event 有 monotonic sequence。

推荐结构：

struct CriticalEventSlot {    std::atomic<std::uint64_t> sequence;    std::atomic<std::uint32_t> flags;    FixedCriticalEventPayload payload;};

17.2 单消费者规则

1. Critical event 允许多生产者：AudioOutputThread / VideoPresentationThread / P5/P6 callback。2. Critical event 只允许一个消费者：GraphOwnerThread。3. 禁止 EvidenceBuildThread / MetricsThread / ObserverThread 直接消费 critical slot。4. GraphOwnerThread 消费后可派发 evidence/metrics。5. Debug guard 必须检测非 GraphOwnerThread 消费行为。

17.3 消费流程

1. GraphOwnerThread acquire load sequence_before。2. acquire load flags。3. copy payload。4. fetch_and(~consumed_mask, acq_rel) 只清除已消费 bit。5. acquire load sequence_after。6. 如果 sequence_after != sequence_before，说明并发写入发生，必须重新扫描 slot。7. 如果 overwrite counter 增加，emit CriticalEventOverwriteDetected evidence。

17.4 Critical event flag memory order

hot path 写：

fetch_or(..., memory_order_release)

GraphOwnerThread 读：

load(memory_order_acquire)

清除已消费 bit：

fetch_and(~consumed_mask, memory_order_acq_rel)

禁止：

exchange(0, memory_order_acq_rel)

### 18. C++ Memory Order Contract

18.1 Snapshot publication

GraphOwnerThread:  build immutable snapshot  release store latest_snapshot_ptrsnapshot():  acquire load latest_snapshot_ptr

18.2 Command queue

enqueue：

release

dequeue：

acquire

如果使用项目已有 lock-free queue，其语义不得弱于 acquire/release。

18.3 Attachment completion queue

producer push completion event：

release

GraphOwnerThread pop completion event：

acquire

Completion event payload 发布后不可变。

### 19. Exception Safety Contract

19.1 Public P7 runtime functions

P7 public runtime functions 必须：

### 1. noexcept。2. 返回 Result<T, PlaybackGraphError> 或 AsyncResult。3. 不抛异常。4. 不跨 module boundary 传播 C++ exception。

19.2 Allocation failure

### 1. Hot path 不分配，因此不产生 bad_alloc。2. Non-hot path 若分配失败，返回 OutOfMemory typed error。3. noexcept 函数内部不得让异常逃逸。4. 如果第三方调用可能抛异常，必须捕获并转换为 typed error。

19.3 Exception guarantees

GraphOwnerThread state mutation:  strong guarantee where possible。  failure before commit 不改变 graph state。  commit 后失败必须产生 recovery/fail closed evidence。Close path:  no-throw best effort。  不允许 close 因异常中断。

### 20. ABI Boundary Contract

如果 P7/P4/P5/P6 在同一静态链接模块中：

内部 C++ 类型可正常使用，但 public graph header 仍不得暴露 platform/Qt/FFmpeg/WASAPI 类型。

如果跨 DLL：

### 1. 不跨 DLL 暴露 STL 容器。2. 不跨 DLL 传播 C++ exception。3. GenerationStamp 必须保持标准布局。4. public enum 必须显式指定 underlying type。5. public struct 不包含虚函数。6. ABI version 必须进入 capability/evidence。

硬规则：

enum class PlaybackGraphState : std::uint32_t;enum class PlaybackGraphError : std::uint32_t;enum class AsyncResultState : std::uint32_t;enum class AttachmentCompletionStatus : std::uint32_t;

ABI 相关指针/句柄：

跨 ABI 边界的 opaque handle 使用 std::uintptr_t 或项目统一 handle 类型。public struct 按 64-bit alignment 明确 padding。

### 21. Logging Boundary

Hot path 禁止任何非实时安全日志。

禁止在以下线程直接调用 logging API：

AudioOutputThreadVideoPresentationThreadP5/P6 high-frequency callback

禁止：

LOG(INFO)printfQString formattingstd::ostringstream动态分配日志字段带锁 logger文件 IO logger

允许：

### 1. 写入 preallocated compact event。2. GraphOwnerThread 或 EvidenceBuildThread 后续转换为 evidence/log。3. DebugDeterministicPreset 下由 fake sink 收集。

### 22. Performance Budget

22.1 Budgets

P7 必须定义性能预算，至少包含：

command_accept_budget_mssnapshot_read_budget_ushot_path_event_submit_budget_usseek_to_first_frame_budget_mspause_command_to_paused_budget_msresume_to_playing_budget_mstrack_switch_budget_msrecovery_budget_msclose_budget_msav_sync_decision_budget_us

默认目标：

snapshot_read_budget_us: <= 100ushot_path_event_submit_budget_us: <= 10uscommand_accept_budget_ms: <= 2mspause_command_to_paused_budget_ms: <= 100msresume_to_playing_budget_ms: <= 250ms local / policy-defined streamingseek_to_first_frame_budget_ms: <= 500ms local / policy-defined streamingtrack_switch_budget_ms: <= 800ms local / policy-defined streamingclose_budget_ms: <= close_timeout_ms

默认值依据：

snapshot_read_budget_us 100us:  约为 16.67ms 视频帧预算的 0.6%。hot_path_event_submit_budget_us 10us:  约为 1ms audio callback period 的 1%。command_accept_budget_ms 2ms:  低于人类明显感知阈值，保证 UI 命令响应轻。pause_command_to_paused_budget_ms 100ms:  交互可感知上限。resume_to_playing_budget_ms 250ms:  本地/轻缓存场景目标，流媒体由 policy 扩展。seek_to_first_frame_budget_ms 500ms:  本地文件假体集成目标，不替代真实 P10 产品级预算。track_switch_budget_ms 800ms:  解码器/管线重启的 fake/integration 预算。

22.2 Measurement

必须由：

FakeClockDeterministicSchedulerGraphMetricsCollectorPerformanceBudgetTool

测量。

记录：

command_start_timecommand_accept_timephase_start_timephase_end_timefirst_frame_after_seek_timesnapshot_read_durationhot_path_event_submit_duration

超预算：

### 1. emit PerformanceBudgetExceeded evidence。2. increment performance_budget_exceeded_count。3. DebugDeterministicPreset 下可使测试 fail。

### 23. Seek / Flushing / Resource Retire

23.1 Seek flow

1. Receive seek command.2. Apply seek coalescing.3. Validate seek target.4. If EosReached, reset EOS/drain state.5. Record pre_seek_play_state.6. Allocate command_id/correlation_id.7. Allocate new SeekEpoch.8. Atomically install seek barrier and stop-publish gate.9. Publish pending GenerationStamp.10. Old video/audio publication is blocked by barrier.11. Mark stale video/audio resources retired.12. Enqueue retired resources into GraphResourceRetireQueue.13. Start background recycle for retired resources.14. Start async source seek if required.15. Start async demux seek if required.16. Start async video flush if selected video exists.17. Start async audio flush if selected audio exists.18. Enter Flushing.19. Await source/demux/audio/video flush/seek completion through PhaseBitset.20. Do not wait for background recycle unless recycle_blocking_required.21. On required phase failure or timeout, enter domain recovery.22. Reset clock reference.23. Allocate new ClockEpoch.24. Publish final post-seek GenerationStamp.25. Re-prime selected pipelines.26. If insufficient data, enter Buffering.27. Release seek barrier.28. Update P4 timeline through P4StateSyncAdapter.29. Resume according to pre_seek_play_state and play_after_seek.30. Emit seek completed evidence.

23.2 Seek phase timeout

每个 phase 必须有独立 timeout：

Phase

Timeout parameter

默认目标

Source Seek

source_seek_timeout_ms

policy-defined, local default 500ms

Demux Seek

demux_seek_timeout_ms

policy-defined, local default 500ms

Audio Flush

audio_flush_timeout_ms

policy-defined, local default 200ms

Video Flush

video_flush_timeout_ms

policy-defined, local default 200ms

Total Seek

seek_timeout_ms

policy-defined

超时处理：

### 1. Emit SeekPhaseTimeout evidence，包含 phase、duration、operation_key。2. 进入 domain-specific recovery。3. 不回滚已完成 phase。4. 不复用 timed out operation_id。5. late completion 按 tombstone 处理。

23.3 Seek logical completion

Seek 逻辑完成条件：

1. Source seek 完成或 source seek not required。2. Demux seek 完成或 demux seek not required。3. Audio flush 完成或 audio not selected。4. Video flush 完成或 video not selected。5. New GenerationStamp 发布。6. Clock reference 重置。7. Required reprime 已完成或进入 Buffering。

Seek 物理旧资源完成条件：

### 1. 旧资源已标记 retired。2. 旧资源已进入 GraphResourceRetireQueue。3. 后台 recycle 可继续进行。

规则：

Seek 逻辑完成后即可进入 Buffering/Playing/Paused。旧资源 recycle 不阻塞常规 seek。

23.4 Barrier semantics

Install seek barrier 是原子语义：

install barrier == stop-publish gate + stale generation marker + operation boundary

因此不再存在“barrier 已安装但旧数据仍发布”的窗口。

23.5 Recycle 不阻塞常规 seek

默认规则：

1. Seek 完成不等待旧 GPU/video resource recycle 完成。2. 旧资源进入 GraphResourceRetireQueue。3. P5 后台 recycle stale resources。4. Recycle completion / timeout 通过 evidence/metrics 记录。5. Recycle timeout 默认进入 ResourceRecycleDomain recovery，但不得回滚已完成 seek。

只有以下情况允许阻塞 seek：

1. P5 reports surface_pool_exhausted。2. P5 contract declares synchronous_recycle_required_for_reprime。3. retired_resource_count >= max_background_retired_resource_count。4. protected content path requires bounded recycle before new frame exposure。

阻塞时必须有 timeout：

physical_retire_enqueue_timeout_msphysical_recycle_timeout_ms

23.6 Flushing command behavior

Flushing 状态下：

close -> immediate cancel and close winsfatal recover -> enter recoverynew seek -> collapse latest or cancel current seek by policypause -> pending post-flush pauseresume -> pending/reject by policytrack switch -> pending/reject by policysnapshot -> immediate

### 24. Drain / EOS

24.1 EOS drain flow

1. Source signals EOS.2. Mark source_eos_seen.3. Continue selected video drain.4. Continue selected audio drain.5. Wait P5 VideoPipelineDrained if selected video exists.6. Wait P5 LastFramePresented if selected video exists and presentation was active.7. Wait P6 AudioPipelineDrained or OutputDrained if selected audio exists.8. Enter EosReached.9. Apply P4 state sync.10. Emit EOS evidence.

24.2 LastFramePresented / OutputDrained

P5 必须提供：

VideoPipelineDrainedLastFramePresentedRendererQueueEmptyPresentQueueCommitted

P6 必须提供：

AudioPipelineDrainedOutputDrainedDeviceBufferDrained

P7 判断 EOS 不得只看 queue length。

24.3 Drain cancel by seek

Seek during drain：

1. close priority check.2. cancel drain barrier.3. async cancel audio drain if selected.4. async cancel video drain if selected.5. async cancel demux drain if applicable.6. wait cancel completion through event-driven phase.7. flush partial drained state.8. clear drain completion flags.9. enter seek flow.

24.4 SourceEosMismatch

SourceEosMismatch 表示：

source/demux 已声明 EOS，但 selected audio/video pipeline 在 drain_timeout 内既没有完成 drain，也没有报告可恢复 failure，并且 queue/evidence 状态与 EOS 声明矛盾。

### 25. Rebuild Granularity Matrix

25.1 Predicate computation source-of-truth

Rebuild predicates 必须代码化，不能只写文档。

必须存在：

rebuild_granularity_matrix.hpp/cpp

或等价单一 source-of-truth。

每个 predicate 必须有：

nameinput snapshotdata sourcecalculationcached valueevidence field

25.2 Predicate definitions

Predicate

Calculation

Data source

shared_demux_required

demux_attachment.owns_track_routing() or track_switch_requires_demux_retune

DemuxAttachmentState / TrackManifest

source_reopen_required

source_session.invalid() or source_token_invalid or source_identity_changed or fatal_connection_loss or resume_connection_invalid

SourceResumeVerifier / SourceSession

clock_reset_required

master_clock_changed or output_clock_invalidated or seek_completed or recovery_changed_output_domain or system_resume

GraphClockCoordinator

video_surface_recycle_required

stale_frame.has_resource_lease or stale_frame.has_decoder_surface or stale_frame.has_renderer_reference or P5 reports recycle_required

VideoFrameLease / P5 recycle status

recycle_blocking_required

surface_pool_exhausted or synchronous_recycle_required_for_reprime or protected_resource_exposure_risk or retired_resource_count >= max_background_retired_resource_count

P5 state / ResourceRetireQueue

audio_generation_required

selected_audio_track_changed or audio_decode_rebuilt or audio_output_rebuilt or audio_format_changed or audio_device_changed

AudioAttachmentState

video_generation_required

selected_video_track_changed or video_decode_rebuilt or video_render_rebuilt or HDR/display mode requires reconfiguration

VideoAttachmentState

graph_generation_required

topology_changed or graph_invariant_broken or selected source/demux architecture changed

PlaybackGraphTopology

25.3 Deterministic matrix

Trigger

GraphGeneration

SourceGeneration

DemuxGeneration

AudioGeneration

VideoGeneration

ClockEpoch

ResourceRecycleEpoch

AudioDeviceLost

-

-

-

+

-

+

-

AudioDecodeFailed

-

-

+ if shared_demux_required

+

-

+ if clock_reset_required

-

VideoDecoderFailed

-

-

-

-

+

+ if clock_reset_required

+ if video_surface_recycle_required

VideoRenderDeviceLost

-

-

-

-

+

+

+

DisplayModeChanged

-

-

-

-

+

+

+

HdrModeChanged

-

-

-

-

+

+

+

AudioTrackSwitch

-

-

+ if shared_demux_required

+

-

+

-

VideoTrackSwitch

-

-

+ if shared_demux_required

-

+

+

+ if video_surface_recycle_required

SourceReadTimeoutRecoverable

-

+ if source_reopen_required

+ if source_reopen_required

+ if source_reopen_required

+ if source_reopen_required

+ if source_reopen_required

+ if source_reopen_required

SourceReopen

-

+

+

+

+

+

+

SeekCompleted

-

-

-

-

-

+

+ if stale resources retired

SystemResume

-

+ if source_reopen_required

+ if source_reopen_required

+ if audio_generation_required

+ if video_generation_required

+

+ if video_surface_recycle_required

GraphInvariantBroken

+

policy

policy

policy

policy

+

+

禁止使用“可能”作为实现依据。所有条件必须转成以上谓词。

### 26. Physical Resource Retire / Recycle

26.1 Resource lifecycle

ActiveRetiredRecycleRequestedRecycleCompletedRecycleTimedOutRecycleFailedLateRecycleAckDiscardedDuplicateAckDetected

26.2 RecycleBarrier 状态

NotStartedMarkingStaleRequestingRecycleWaitingForAckCompletedTimedOutBrokenByCloseLateAckDiscardedDuplicateAckDetectedFailed

合法转移：

NotStarted -> MarkingStaleMarkingStale -> RequestingRecycleRequestingRecycle -> WaitingForAckWaitingForAck -> CompletedWaitingForAck -> TimedOutWaitingForAck -> BrokenByCloseTimedOut -> FailedBrokenByClose -> LateAckDiscarded if late ack arrivesCompleted -> DuplicateAckDetected if duplicate ack arrivesDuplicateAckDetected -> Completed

DuplicateAckDetected 必须生成 warning evidence，不能静默吞掉。

26.3 Recycle loop prevention

max_lease_recycle_retry_countmax_recycle_barrier_retry_countmax_background_retired_resource_count

超过限制进入 RecoveryFuseTriggered 或 FailClosed。

### 27. Recovery / Health Observation

27.1 Playback recovery domains

SourceDomainDemuxDomainVideoDecodeDomainVideoRenderDomainAudioDecodeDomainAudioOutputDomainClockDomainResourceRecycleDomainGraphDomainCombinedDomain

27.2 Health observation only domains

EvidenceHealthDomainMetricsHealthDomainObserverHealthDomain

Evidence/Metrics/Observer failure 不触发用户可见 playback recovery。

它们只允许：

1. mark GraphHealth DegradedDiagnostics。2. drop non-critical evidence/metrics according to policy。3. reconnect/reinitialize background sink。4. never stop playback solely because evidence/metrics sink failed。

27.3 Recovery reason enum

SourceReadTimeoutSourceSeekTimeoutSourceFatalEndSourceConnectionLostSourceResumeReconnectRequiredSourceResumeReconnectFailedDemuxCorruptionDemuxRetuneFailedVideoDecodeCrashVideoDecodeUnsupportedVideoHardwareToSoftwareFallbackUnsupportedVideoRenderDeviceLostVideoRenderFailedVideoSurfaceRecycleTimeoutDisplayModeChangedHdrModeChangedAudioDecodeFailedAudioOutputDeviceLostAudioOutputFailedAudioUnderrunFatalClockDriftFatalClockSnapshotInvalidQueueCorruptionGenerationInvariantBrokenGraphInvariantBrokenGraphRevisionMismatchSeekTimeoutSeekPhaseTimeoutTrackSwitchTimeoutDrainTimeoutSourceEosMismatchEvidenceSinkFailedMetricsSinkFailedObserverDeliveryFailedCombinedDomainFailureDetectedRecoveryFuseTriggeredBuildRetryExhausted

27.4 Secondary domain verification

Combined-domain failure 恢复 primary domain 后，必须执行 secondary verification。

流程：

1. Primary domain recovery completed。2. For each secondary domain:   query attachment state   query queue state   query last error   query generation stamp compatibility   query clock compatibility if relevant3. Healthy -> SecondaryVerifiedHealthy。4. Degraded but playable -> SecondaryVerifiedDegraded + GraphHealth update。5. Failed -> escalate according to recovery policy。6. Timeout -> SecondaryVerificationTimeout。

### 28. System Suspend / Resume / Source Reconnect

28.1 系统状态

SystemSuspendingSystemSuspendedSystemResuming

SystemSuspend flow：

current playback state→ SystemSuspending→ hold clock→ pause/throttle selected pipelines by policy→ SystemSuspended

SystemResume flow：

SystemSuspended→ SystemResuming→ ClockEpoch++→ query P2/P3 source connection state→ query P6 audio device→ query P5 display/HDR/render state→ if source invalid: async source reconnect/reopen→ if display/HDR changed: VideoRenderDomain recovery/reconfiguration→ if audio device changed: AudioOutputDomain recovery→ if source reconnect succeeded: enter Buffering before Playing→ return to Paused / Buffering / Playing according to pre_suspend_state

28.2 SourceResumeVerifier

P7 必须新增：

SourceResumeVerifier

职责：

1. query_source_connection_state。2. detect source token invalidation。3. detect live session expired。4. request source reconnect/reopen through P2/P3。5. produce SourceResumeVerified / SourceResumeReconnectRequired / SourceResumeReconnectFailed evidence。

Source resume 不允许 P7 直接打开 URL。必须通过 P2/P3 SourceSession 合同。

### 29. Evidence / Metrics / Close Flush Order

29.1 Evidence sampling

CriticalEvidence：

state transitionseek accepted/completed/collapsedrecovery start/endfailuretimeoutstale discard summaryphysical recycle timeoutduplicate recycle ackcloselate callback/recycle discardprotected content failpending command replay/reject/expire/abortsource resume reconnectStampReadUnstablecritical event overwriteoperation tombstone hit

SampledEvidence：

repeated underrunqueue depth updateAV sync decisionmetrics tick

29.2 Evidence retention

必须定义：

max_evidence_recordsmax_evidence_bytesmax_retention_duration_mscritical_evidence_reserved_slotsgeneration_tuple_retention_count

29.3 Close flush order

Close finalization 必须采用两阶段冻结：

1. stop event ingestion。2. freeze metrics snapshot in memory。3. emit final evidence record referencing metrics_snapshot_id。4. flush evidence records。5. flush metrics snapshot。6. publish final close snapshot。

Evidence 与 Metrics 通过 id 关联，不允许互相持有悬空引用。

### 30. Privacy Redaction

Source identity redaction 必须标准化。

允许：

scheme + host hashpath basename redactedquery removedtoken/cookie removedHMAC-SHA256 with app-local diagnostic salt if stable identity needed

禁止：

完整 URLquery tokencookieprovider access key

### 31. P5 / P6 Attachment Contract

31.1 VideoPipelineAttachment

所有耗时操作返回 AttachmentAsyncToken，通过 AttachmentCompletionEvent 完成。

query_video_capability(input_context) -> VideoCapabilityResultattach_video_track(track_id, generation_stamp) -> AttachmentAsyncTokendetach_video_track(reason) -> AttachmentAsyncTokenprime_video(target_timeline, generation_stamp) -> AttachmentAsyncTokenstart_video_presentation(generation_stamp) -> AttachmentAsyncTokenpause_video_presentation() -> AttachmentAsyncTokenresume_video_presentation(generation_stamp) -> AttachmentAsyncTokenhold_video_presentation(clock_epoch) -> AttachmentAsyncTokenflush_video(reason, generation_stamp) -> AttachmentAsyncTokenseek_video(target_timeline, seek_epoch, generation_stamp) -> AttachmentAsyncTokenswitch_video_track(track_id, video_generation, generation_stamp) -> AttachmentAsyncTokendrain_video(generation_stamp) -> AttachmentAsyncTokencancel_video_drain(reason) -> AttachmentAsyncTokenrequest_video_retire(retire_set_id, stale_generation_stamp) -> AttachmentAsyncTokenrequest_video_recycle(recycle_barrier_id, stale_generation_stamp) -> AttachmentAsyncTokenquery_video_recycle_status(recycle_barrier_id) -> VideoRecycleStatusapply_av_sync_decision(decision) -> AVSyncDecisionResultget_video_clock_snapshot() -> VideoClockSnapshotget_video_queue_snapshot() -> VideoQueueSnapshotget_video_attachment_state() -> VideoAttachmentStateclose_video_attachment() -> AttachmentAsyncToken

P5 必须发出：

VideoPipelineDrainedLastFramePresentedRendererQueueEmptyPresentQueueCommittedVideoRecycleCompletedVideoRecycleTimedOut

31.2 AudioPipelineAttachment

query_audio_capability(input_context) -> AudioCapabilityResultattach_audio_track(track_id, generation_stamp) -> AttachmentAsyncTokendetach_audio_track(reason) -> AttachmentAsyncTokenprime_audio(target_timeline, generation_stamp) -> AttachmentAsyncTokenstart_audio_output(generation_stamp) -> AttachmentAsyncTokenpause_audio_output() -> AttachmentAsyncTokenresume_audio_output(generation_stamp) -> AttachmentAsyncTokenflush_audio(reason, generation_stamp) -> AttachmentAsyncTokenseek_audio(target_timeline, seek_epoch) -> AttachmentAsyncTokenswitch_audio_track(track_id, audio_generation, generation_stamp) -> AttachmentAsyncTokendrain_audio(generation_stamp) -> AttachmentAsyncTokencancel_audio_drain(reason) -> AttachmentAsyncTokenget_audio_clock_snapshot() -> AudioClockSnapshotget_audio_queue_snapshot() -> AudioQueueSnapshotget_audio_attachment_state() -> AudioAttachmentStatequery_silence_fill_capability() -> SilenceFillCapabilityquery_micro_adjust_capability() -> MicroAdjustCapabilityrequest_audio_micro_adjust(adjustment_ratio, clock_epoch) -> AttachmentAsyncTokenclose_audio_attachment() -> AttachmentAsyncToken

P6 必须发出：

AudioPipelineDrainedOutputDrainedDeviceBufferDrained

AudioQueueSnapshot 必须包含：

packet_countbytesduration_msunderrun_countlast_packet_stamp

VideoQueueSnapshot 必须包含：

frame_countbytesduration_msoldest_frame_stampnewest_frame_stamp

31.3 AVSyncDecisionResult

AcceptedAppliedExpiredUnsupportedRejectedInvalidStampRejectedWrongDeadline

### 32. Error Model

P7 error 必须 typed。

InvalidStateInvalidCommandInvalidSeekTargetSeekCollapsedPreviewSeekCollapsedCommitSeekCollapsedEosReachedRequiresSeekPendingDuringRecoveryRejectedDuringRecoveryPendingCommandExpiredPendingCommandReplayAbortedCancelledByCloseSourceOpenFailedSourceReadFailedSourceSeekFailedSourceEosMismatchSourceConnectionLostSourceResumeReconnectRequiredSourceResumeReconnectFailedGraphBuildFailedBuildRetryExhaustedCapabilityNegotiationFailedPolicyInvalidPolicyClampedProtectedContentUnsupportedProtectedPathRequiredProtectedOutputUnavailableAudioAttachmentFailedVideoAttachmentFailedDemuxAttachmentFailedAttachmentAsyncContractMissingAttachmentCompletionTimeoutLateAttachmentCompletionDiscardedDuplicateAttachmentOperationIdAudioFlushFailedVideoFlushFailedVideoRecycleFailedAudioTrackSwitchFailedVideoTrackSwitchFailedClockPolicyFailedAVSyncDecisionExpiredAVSyncDecisionUnsupportedRejectedInvalidStampRejectedWrongDeadlineBufferingTimeoutRebufferingTimeoutStalledTimeoutSeekPhaseTimeoutDrainFailedDrainCancelledLastFramePresentedTimeoutOutputDrainedTimeoutRecoveryFailedSecondaryVerificationFailedSecondaryVerificationTimeoutCombinedDomainFailureRecoveryFuseTriggeredCloseFailedTimeoutDeadlineMissedCancelledClosedGraphContractMissingContractHeaderMismatchContractBehaviorMismatchForbiddenBoundaryViolationQueueOverflowPendingQueueOverflowObserverQueueOverflowCriticalEventOverflowCriticalEventOverwriteDetectedLateCallbackDiscardedLateRecycleAckDiscardedDuplicateRecycleAckDetectedUnsupportedPlaybackRateGenerationWrapRiskGenerationStampMismatchStampReadUnstableGenerationTupleMissingGenerationStamp64AtomicNotLockFreeGraphInvariantBrokenGraphRevisionMismatchP4StateSyncFailedOutOfMemoryExceptionCapturedAbiBoundaryViolationHotPathLoggingViolationHotPathAllocationViolationHotPathBlockingViolationPerformanceBudgetExceededPublicHeaderPuristFailedMissingCarryoverPreflightFailed

### 33. Architecture Guard

33.1 Forbidden dependency scan

分层：

1. #include scan：public graph headers 禁止 include FFmpeg/WASAPI/Qt/D3D/DXGI/Win32。2. type token scan：代码 token 中禁止出现 forbidden concrete types。3. comment-only occurrence 不 fail，可 warning。4. AST / dependency graph guard 优先于纯文本搜索。5. path-scoped exceptions 必须显式登记，不得默认放行。

33.2 Forbidden public types

AVFormatContextAVCodecContextAVFrameAVPacketSwrContextSwsContextIAudioClientIAudioRenderClientIMMDevicewindows.hd3d11.hdxgi.hComPtrQObjectQWidgetQStringQVariantQUrlQML

33.3 Public include guard

必须 PASS：

public include contains no fakepublic include contains no test_supportpublic include contains no test_contractpublic include contains no deterministic_fake_scheduler

33.4 Public Header Purist Check

CI 必须新增：

Public Header Purist Check

检查方式：

1. 创建 dummy cpp。2. 只 include P7 public headers。3. 不定义任何测试宏。4. 不包含 backend/tests 路径。5. 不链接 fake/test_support。6. 编译必须 PASS。7. 如果 public header 依赖 KIVO_TEST_SUPPORT、fake、deterministic scheduler、test macro，直接 FAIL。

示例：

#include <kivo/video/playback_graph/session/playback_session_contract.hpp>#include <kivo/video/playback_graph/contracts/generation_stamp.hpp>#include <kivo/video/playback_graph/command/graph_command_conflict_matrix.hpp>int main() { return 0; }

### 34. Tests / Fake Harness

Private test support only：

backend/tests/video/playback_graph/test_support/**

每个测试必须：

### 1. 创建全新 PlaybackSession。2. 创建全新 FakeClock。3. 创建全新 ManualEventPump。4. 创建全新 fake attachments。5. 不共享 mutable global state。6. 不依赖测试顺序。7. 不使用 sleep。

必备测试分类：

State Transition TestsCommand Conflict Matrix TestsAsync Attachment TestsPhase Bitset TestsGenerationStamp / Seqlock TestsCritical Event TestsSeek / Recycle TestsEOS Drain TestsSystem Suspend/Resume TestsRecovery TestsPerformance Budget TestsArchitecture Guard TestsContract Gap Tests

必备场景：

audio_only_open_start_closevideo_only_open_start_closeopen_start_close_basicrapid_seek_collapseseek_while_pausedseek_during_flushingseek_phase_timeout_sourceseek_phase_timeout_demuxseek_phase_timeout_audio_flushseek_phase_timeout_video_flushseek_recycle_nonblockingseek_recycle_blocks_only_when_surface_pool_exhaustedseek_logical_completion_before_recycle_completioneos_seekpause_after_eos_noopresume_after_eos_requires_seekplay_after_eos_policyeos_waits_last_frame_presentedeos_waits_output_drainedtrack_switch_preserve_unaffected_pipelinesame_track_switch_noopstale_frame_packet_discardstamp_read_unstable_drops_unsubmitted_framestamp_read_unstable_drops_unwritten_packetstamp_read_unstable_does_not_retract_submitted_framegeneration_stamp_wrap_fake_small_widthseqlock_concurrent_read_writecritical_event_slot_no_abacritical_event_single_consumercritical_event_sequence_wrap_fake_small_widthcritical_event_overwrite_detectedphysical_resource_retire_queuelate_callback_after_closelate_recycle_ack_after_closedomain_isolated_recoverycombined_domain_failuresecondary_domain_verificationpending_command_replay_after_recoverypending_command_replay_abortpending_command_replay_orderrebuffering_pause_intentrebuffering_pause_intent_timeout_in_rebufferingrebuffering_pause_intent_timeout_in_pausingcan_hold_immediately_truecan_hold_immediately_falserecovery_fuseav_sync_decision_deliverybuffering_rebuffering_stalled_debouncestalled_validation_tolerancequeue_budget_dimension_disabled_explicitqueue_budget_half_config_raw_zero_rejectedmicro_adjust_capability_refresh_on_buildmicro_adjust_capability_refresh_on_track_switchmicro_adjust_capability_refresh_on_device_changepolicy_preset_validationsafety_critical_tuning_validationobserver_backpressureevidence_sampling_privacysubtitle_sync_anchor_after_seek_track_switch_recoverydata_path_control_plane_boundaryp4_state_sync_semantic_onlyp4_revision_owner_is_p4graph_revision_mismatchdrain_cancel_by_seeksystem_suspend_resumesystem_resume_source_reconnectsystem_resume_hdr_changeasync_attachment_operation_id_uniquenessasync_attachment_cancel_best_effortasync_attachment_completion_tombstoneasync_attachment_completion_orderingattachment_completion_timeoutattachment_completion_racephase_bitset_concurrencyqueue_snapshot_bounded_readgeneration_stamp_tuple_registry_retentioncpp_memory_order_publicationexception_safety_noexcept_resulthot_path_static_guardhot_path_runtime_allocation_guardhot_path_logging_forbiddenperformance_budget_fake_seekperformance_budget_exceeded_evidencepublic_header_purist_checkcontract_header_diff_toolcontract_behavior_test_suitecontract_gap_scannerrebuild_predicate_computationrebuild_matrix_codegencommand_conflict_matrix_codegenfull_command_conflict_matrixclosed_through_closing_onlyfailed_fast_closingbuildfailed_fast_closing

### 35. STOP Conditions

设计 STOP：

1. P2/P3 SourceSession / Source seek / Source close / SourceOperationToken 合同缺失。2. P2/P3 Source resume/reconnect/reopen state query 合同缺失。3. P4 command/state/recovery/revision/state sync 合同缺失。4. P4 stable PlaybackCommandId 缺失。5. P4 不拥有 PlaybackRevision 分配权。6. P5 video pipeline async token/completion event 合同缺失。7. P5 video pipeline flush/seek/track switch/drain/cancel drain 合同缺失。8. P5 normalized timeline stamp 合同缺失。9. P5 physical retire/recycle contract 缺失。10. P5 LastFramePresented / PipelineDrained event 缺失。11. P5 AVSync decision application contract 缺失。12. P5 queue duration snapshot 缺失。13. P5 bounded queue snapshot read 无法保证且没有 async QueueSnapshotEvent。14. P5 capability query before attachment 缺失。15. P6 audio pipeline async token/completion event 合同缺失。16. P6 audio pipeline flush/seek/track switch/drain/cancel drain 合同缺失。17. P6 OutputDrained / PipelineDrained event 缺失。18. P6 normalized timeline stamp 合同缺失。19. P6 queue duration snapshot 缺失。20. P6 bounded queue snapshot read 无法保证且没有 async QueueSnapshotEvent。21. P6 underrun / silence-fill capability 合同缺失且无法 typed fallback。22. P6 micro-adjust capability 缺失但 policy 要求启用。23. 需要修改 P2/P3/P4/P5/P6 frozen contract 才能继续。24. 需要新增真实 FFmpeg/WASAPI/renderer/provider 才能继续。25. 需要实现字幕才能继续。26. 需要 Qt/UI 参与才能继续。27. 无法保证 Closed 只通过 Closing 进入。28. 无法保证 Failed/BuildFailed fast closing 安全。29. 无法保证 GenerationStamp opaque id + tuple registry。30. 无法保证 64-bit atomic lock-free。31. 无法保证 seqlock retry 最多 3 次。32. 无法保证 seqlock 读取失败时 drop 未提交 frame/packet。33. 无法保证 critical event no ABA。34. 无法保证 critical event single consumer。35. 无法保证 realtime preallocation。36. 无法保证 no hot path logging/allocation/blocking。37. 无法保证 async attachment completion event。38. 无法保证 attachment operation id uniqueness。39. 无法保证 attachment timeout/tombstone。40. 无法保证 phase bitset 无 deadlock。41. 无法保证 pending command replay / replay abort。42. 无法保证 seek phase timeout。43. 无法保证 seek recycle nonblocking。44. 无法保证 old generation frame/packet discard。45. 无法保证 physical resource retire/recycle。46. 无法保证 close 抢占 in-flight command。47. 无法保证 late callback after close safety。48. 无法保证 late recycle ack after close safety。49. 无法建立 evidence correlation。50. 无法建立 evidence privacy redaction。51. 无法验证 safety-critical tuning 合法范围。52. 无法区分 DataPath DAG 和 ControlPlane。53. public include 必须暴露 fake/test_support 才能编译。54. 无法通过 Public Header Purist Check。55. 无法通过 Contract Gap Scanner。56. 无法输出 Missing-Carryover Report。

Pre-flight fail 不归入设计 STOP，单独输出 PREFLIGHT_FAIL。

### 36. Exit Criteria

P7 必须满足：

G1  PlaybackGraphBuilder 可以创建 DataPath DAG graph topology。G2  ControlPlane 与 DataPath 明确分离。G3  PlaybackSession 可以 open/close。G4  所有 Closed 转移都经过 Closing。G5  Failed / BuildFailed / NotCreated / Created fast closing 安全。G6  close while Closing/Closed 是 NoOp 或 last close result。G7  P4StateSyncAdapter 生效。G8  P4StateSyncAdapter 只做语义校验与 revision 返回，不查询物理状态。G9  PlaybackRevision 只由 P4 分配。G10 GraphRevisionMismatch 可检测。G11 P7CommandIntentSidecar 生效，不修改 P4 frozen header。G12 SeekRequest 明确携带 play_after_seek。G13 AsyncResult 是非阻塞 token，无 get/wait。G14 AsyncResult lifecycle 由 GraphOwnerThread/PlaybackSession 管理。G15 Hot path 不访问 command registry。G16 Sidecar 生命周期覆盖 command 完成前。G17 Full Command Conflict Matrix 覆盖所有状态。G18 Command Conflict Matrix source-of-truth 代码化。G19 Matrix tests 100% 覆盖 state × command。G20 Flushing command behavior 明确。G21 Pending command queue 容量/超时/淘汰/replay 生效。G22 Pending replay failure 可中断后续 replay。G23 P5/P6 async token + completion event 生效。G24 Attachment operation id uniqueness PASS。G25 Attachment cancel best-effort/idempotent PASS。G26 Attachment tombstone / late completion discard PASS。G27 AttachmentCompletionTimeout 可触发 domain recovery。G28 PhaseBitset 同步机制 PASS。G29 PhaseBitset timeout PASS。G30 Queue snapshot bounded read 或 async snapshot event 生效。G31 GenerationStamp 使用 opaque id + tuple registry。G32 GenerationStamp 禁止 hash。G33 GenerationStamp per-stream audio/video 分离。G34 128-bit atomic 禁止。G35 64-bit atomic lock-free static assert PASS。G36 Seqlock publication 生效。G37 Seqlock retry 上限为 3。G38 Seqlock 读取失败 drop 未提交 frame/packet。G39 Critical event no ABA。G40 Critical event single consumer。G41 Critical event 禁止 exchange(0) 清空整组 flag。G42 Critical event sequence wrap fake test PASS。G43 C++ memory order review PASS。G44 Hot Path 线程集合明确。G45 Hot path 静态扫描 PASS。G46 Hot path runtime allocation/logging guard PASS。G47 Rebuffering pause intent 必须经过 Pausing。G48 Rebuffering pause intent timeout 行为明确。G49 can_hold_immediately 判定明确。G50 EOS pause NoOp / resume requires seek。G51 EOS 等 LastFramePresented / OutputDrained。G52 Stalled validation 使用 jitter tolerance，边界不 flaky。G53 Queue budget 显式维度配置 PASS。G54 raw zero 半配置歧义被 reject。G55 Micro-adjust capability refresh 生效。G56 Safety-critical tuning validation PASS。G57 Rebuild Granularity Predicate 代码化。G58 Rebuild Matrix 无“可能/视情况”，全部使用谓词。G59 Seek phase timeout PASS。G60 Seek 逻辑完成定义明确。G61 Seek recycle 默认不阻塞。G62 Seek 仅在 surface_pool_exhausted / synchronous_recycle_required 等条件下等待 recycle。G63 Evidence/Metrics failure 不触发用户可见 playback recovery。G64 SystemSuspending/SystemSuspended/SystemResuming 生效。G65 SystemResume 检查 Source connection state。G66 Source reconnect/reopen 通过 P2/P3 contract。G67 SourceEosMismatch error 存在。G68 DisplayModeChanged/HdrModeChanged recovery reason 存在。G69 Performance Budget 可测量。G70 PerformanceBudgetExceeded evidence 生效。G71 Exception Safety Contract PASS。G72 ABI Boundary Contract PASS。G73 Public include fake/test_support absence PASS。G74 Public Header Purist Check PASS。G75 Contract Gap Scanner PASS。G76 Contract Header Diff Tool PASS。G77 Contract Behavior Test Suite PASS。G78 Private test_support 路径正确。G79 All required integration tests PASS。G80 Architecture guard PASS。G81 Dependency graph guard PASS。G82 Final Report 证据完整。

### 37. Final Report 要求

P7 完成后必须提交 Final Report，至少包含：

1. Branch name2. Base commit3. Final HEAD4. Remote HEAD5. Files changed summary6. Natural family tree created7. P7 public headers list8. P7 private runtime files list9. P7 private test_support files list10. CMake targets added11. Tests executed12. Test result13. Preflight result14. Architecture guard result15. Forbidden include/type scan result16. Public include fake/test_support absence result17. Public Header Purist Check result18. Contract Gap Scanner result19. Contract Header Diff Tool result20. Contract Behavior Test Suite result21. Frozen contract guard result22. P4StateSyncAdapter semantic-only evidence23. P4 revision owner evidence24. GraphRevisionMismatch evidence25. P7CommandIntentSidecar evidence26. AsyncResult lifecycle evidence27. AsyncResult token-only evidence28. command registry hot-path absence evidence29. Closed-through-Closing evidence30. failed fast-closing evidence31. close idempotency evidence32. Full command conflict matrix codegen evidence33. matrix 100% state-command test evidence34. Flushing command behavior evidence35. SeekRequest play_after_seek evidence36. Pending command replay evidence37. Pending replay abort evidence38. Async attachment token/completion evidence39. Attachment operation id uniqueness evidence40. Attachment cancel/tombstone evidence41. AttachmentCompletionTimeout evidence42. PhaseBitset evidence43. Queue snapshot bounded-read evidence44. GenerationStamp opaque id / tuple registry evidence45. GenerationStamp no-hash evidence46. no 128-bit atomic evidence47. 64-bit atomic lock-free static assert evidence48. Seqlock publication evidence49. Seqlock retry count evidence50. StampReadUnstable drop-frame/drop-packet evidence51. Critical event no-ABA evidence52. Critical event single-consumer evidence53. Critical event overwrite evidence54. C++ memory order review result55. Hot Path thread list evidence56. Hot path static guard evidence57. Hot path runtime allocation/logging guard evidence58. Realtime preallocation evidence59. Seek phase timeout evidence60. Seek logical completion evidence61. Seek recycle nonblocking evidence62. Seek recycle blocking-only-when-required evidence63. Rebuffering pause intent evidence64. pause intent timeout evidence65. can_hold_immediately evidence66. EOS play/pause/resume policy evidence67. EOS LastFramePresented / OutputDrained evidence68. Safety-critical tuning validation result69. Stalled jitter tolerance validation evidence70. Queue budget explicit dimension evidence71. Micro-adjust capability refresh evidence72. Rebuild predicate codegen evidence73. Source resume reconnect evidence74. SourceEosMismatch evidence if tested75. Secondary-domain verification evidence76. Evidence/Metrics health observation evidence77. System suspend/resume evidence78. Performance budget result79. PerformanceBudgetExceeded evidence80. Exception safety result81. ABI boundary result82. Contract Gap Registry result83. Contract Gap Report if triggered84. Missing-Carryover Report if triggered85. Exit criteria checklist86. Known non-goals confirmation87. STOP condition not triggered confirmation88. Ready for P8 statement

### 38. P7 不碰的后续内容

38.1 字幕

P7 不做字幕。字幕进入：

P8 Subtitle Plane

P7 只保留 future subtitle sync slot，不实现字幕功能。

38.2 Qt UI

P7 不做 Qt UI。Qt runtime adapter 进入：

P9 Player Runtime Shell / Qt Adapter

38.3 播放列表 / 下一项

P7 只负责单 PlaybackSession。

不负责：

playlist queuenext item orchestrationauto play next episodegapless video playlist

EOS 后是否自动下一项由 P9/P10 runtime shell 决策。

38.4 真实产品级闭环

真实源 + 真实 FFmpeg + 真实 WASAPI + 真实 Renderer 的最终产品级回归进入：

P10 Final Backend Closure

P7 只要求 fake end-to-end integration 闭环和合同级 runtime glue 闭环。

### 39. 最终交付结论

P7 完成后，必须可以明确声明：

Playback graph integrated.Source + P4 + P5 + P6 can run as one coordinated PlaybackSession.All terminal transitions go through Closing.Failed / BuildFailed / NotCreated / Created use explicit fast Closing paths.Close is idempotent in Closing/Closed.P4 state sync is locked to P4StateSyncAdapter.P4StateSyncAdapter is semantic-only and does not validate physical P5/P6/source state.PlaybackRevision is owned only by P4.GraphRevisionMismatch is detectable.P7 command intent sidecar avoids modifying P4 frozen command header.SeekRequest explicitly carries play_after_seek.AsyncResult is token-only and non-blocking.AsyncResult lifecycle is owned by PlaybackSession / GraphOwnerThread.Hot path never accesses command registry.Sidecar lifetime is tied to PlaybackCommandId.Full command conflict matrix covers every graph state.Command conflict matrix is code-generated from one source-of-truth.Pending command replay is deterministic and abortable on failure.P5/P6 attachment methods use async token + completion event semantics.AttachmentOperationId uniqueness is defined.Cancel is best-effort and idempotent.Late completion is handled through tombstone.Attachment completion loss is detected by timeout.PhaseBitset is the only async phase synchronization primitive.GenerationStamp is collision-free, opaque-id based, per-stream, and hot-path safe.GenerationTupleRegistry supports slow-path evidence expansion.No 128-bit atomic is used.64-bit atomic lock-free is required.Windows x86-64 uses seqlock stamp publication.Seqlock retry limit is 3.Seqlock read failure drops current unsubmitted frame/packet.Critical event handling is versioned and ABA-safe.Critical event has multiple producers but exactly one consumer.exchange(0) full flag clear is forbidden.C++ memory order rules are defined.Hot Path threads are explicitly listed.Realtime hot path uses preallocated memory only.Hot path logging/allocation/blocking/mutex/future wait/file IO/network IO/sleep are forbidden.Static and runtime hot-path guards exist.Seek does not wait for stale GPU/video recycle on the normal path.Seek phase timeout exists for source/demux/audio/video phases.Seek logical completion is distinct from background resource recycle.Stale resources enter retire queue and recycle in the background.Recycle only blocks seek when surface pool/protected path/reprime requires it.EOS waits for LastFramePresented / OutputDrained, not only queue empty.SystemResume verifies Source connection and can reconnect through P2/P3.Rebuild granularity is deterministic, predicate-driven, and code-generated.Rebuffering pause intent has a concrete Pausing path and timeout behavior.EOS direct play/pause/resume policy is explicit.Stalled debounce validation is mathematically stable with jitter tolerance.Queue budget validation uses explicit dimension modes and rejects ambiguous raw-zero half-configs.Micro-adjust validation is capability-aware and refreshed on build/track switch/device change.Evidence/Metrics failure does not stop playback.System suspend/resume has explicit graph states.Performance Budget exists and is measurable.Exception Safety Contract exists.ABI Boundary Contract exists.Public include tree contains no fake/test_support API.Public Header Purist Check exists.Contract Gap Scanner / Header Diff Tool / Behavior Test Suite exist.P8 subtitle slot has VideoGeneration/SeekEpoch/ClockEpoch/GenerationStamp.No UI/subtitle-runtime/new decoder/new output/new provider was added.Ready for P8 Subtitle Plane.

END OF KIVO VIDEO P7 PLAYBACK GRAPH INTEGRATION DESIGN V1.9
