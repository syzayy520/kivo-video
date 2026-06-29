# P7A V1.9 to V1.9.1 Carryover Report

**Document type**: automated carryover audit seed / planning evidence
**Generated**: `2026-06-29T01:05:06Z`
**Implementation baseline**: P7 V1.9.1 DOCX
**Historical baseline**: P7 V1.9 DOCX from `C:/Users/huyasi/Downloads`

## Hashes

- V1.9 source DOCX SHA256: `B85D3B4C200445A46469AD4600B97BD0BB409F0DA7D3679BA1E0E44F87529FD8`
- V1.9 baseline Markdown SHA256: `534277B56171E4E4E6FEBBBAB83374C26B31BE6C7411489CC083872BE625F8B4`
- V1.9 rule manifest SHA256: `DCB8237A5DD1DD7004BDC5570B55D5AC5B820E420657CB2731DE4086F643E38F`
- V1.9.1 source DOCX SHA256: `9A52A8B2C14F08F5D804771759B32520B991D7D19963A0DE49B238505D4ED05F`

## Audit Rule

This report does not make implementation decisions by itself. It seeds the no-omission audit by mapping V1.9 section headings to V1.9.1 exact or candidate replacements. Manual review remains required for candidate and unmatched rows before final P7 closure.

## Summary

- preserved_title: 11
- candidate_renamed_or_moved: 21
- manual_review_required: 32
- total_v1_9_rules: 64

## Mapping

| V1.9 rule | V1.9 title | Status | V1.9.1 replacement |
|---|---|---|---|
| 0 | 最终执行口令 | preserved_title | V1.9.1 0 最终执行口令 |
| 1 | Merge Integrity Gate | preserved_title | V1.9.1 2 Merge Integrity Gate |
| 2 | Platform / Toolchain / Language Baseline | candidate_renamed_or_moved | candidate V1.9.1 3 Platform / Toolchain / Build Targets |
| 3 | Scope / Non-goals | preserved_title | V1.9.1 4 Scope / Non-goals |
| 4 | Terminology | manual_review_required | not automatically matched |
| 5 | Pre-flight Checks | manual_review_required | not automatically matched |
| 1 | 当前 repo 必须是 {KIVO_VIDEO_ROOT} 或项目明确登记的真实根目录。2. 当前分支必须是 P7 专用分支。3. 工作区必须 clean。4. 禁止在 main/master 直接施工。5. 禁止夹带 P8/P9/P10 文件。 | manual_review_required | not automatically matched |
| 6 | Contract Gap Handling / Registry | candidate_renamed_or_moved | candidate V1.9.1 33 Contract Gap Levels |
| 1 | STOP。2. 写 Contract Gap Registry。3. 输出 Contract Gap Report。4. 不允许在 P7 偷补真实 P2/P3/P4/P5/P6 能力。 | candidate_renamed_or_moved | candidate V1.9.1 33 Contract Gap Levels |
| 7 | Natural Family Tree | manual_review_required | not automatically matched |
| 1 | 单一职责优先于机械行数。2. public header 可以通过主题子结构体拆分，不允许 _part1/_part2。3. 超过 soft limit 必须在 Final Report 解释原因。4. 超过 hard limit 必须拆分。5. 禁止 misc/common/utils/helpers/manager/all_in_one。 | candidate_renamed_or_moved | candidate V1.9.1 39 Final Report Requirements |
| 8 | Policy / Tuning / Safety Validation | manual_review_required | not automatically matched |
| 1 | 禁止 wall-clock sleep。2. 所有 timestamp 由 FakeClock 提供。3. invalid tuning 必须 reject，不允许 clamp。4. evidence full mode。 | manual_review_required | not automatically matched |
| 9 | Hot Path Contract | preserved_title | V1.9.1 20 Hot Path Contract |
| 10 | P4 Bridge / Command Intent | candidate_renamed_or_moved | candidate V1.9.1 9 P4 Bridge / Revision Boundary |
| 1 | 验证 P4 command/revision/state transition 语义。2. 验证 generation_stamp 与 expected_revision 的逻辑一致性。3. 返回 P4 分配的 PlaybackRevision。4. 返回 P4 PlaybackState。5. 返回 typed failure。 | candidate_renamed_or_moved | candidate V1.9.1 1 observer event2. query_command(command_id)3. snapshot command history window |
| 1 | 查询 P5/P6 queue。2. 查询 source 状态。3. 查询真实 audio/video clock。4. 等待 attachment。5. 判断底层设备是否真实 ready。6. 做 IO。7. 做 blocking wait。8. 分配 P7 资源。 | manual_review_required | not automatically matched |
| 11 | PlaybackSession API / AsyncResult | manual_review_required | not automatically matched |
| 1 | GraphEventSink / observer event。2. CommandCompleted event。3. CommandFailed event。4. snapshot polling。 | candidate_renamed_or_moved | candidate V1.9.1 1 observer event2. query_command(command_id)3. snapshot command history window |
| 1 | CommandCompleted。2. CommandFailed。3. CommandCancelled。4. CancelledByClose。5. PlaybackSession destruction。 | manual_review_required | not automatically matched |
| 1 | Completion event 到达时如果 command_id 已不存在，忽略该 event。2. 必须 emit LateCommandCompletionDiscarded evidence。3. late event 不得复活 command。4. late event 不得访问已销毁 AsyncResult record。 | candidate_renamed_or_moved | candidate V1.9.1 1 observer event2. query_command(command_id)3. snapshot command history window |
| 12 | PlaybackGraph State Machine | preserved_title | V1.9.1 11 PlaybackGraph State Machine |
| 1 | 直接跳 Closed。2. 访问已 invalid 的 P5/P6 attachment。3. 强行 flush 已 fatal 的 pipeline。4. double free。 | manual_review_required | not automatically matched |
| 1 | 保持 pause_intent_pending。2. 等待 Rebuffering 完成或等待 attachment event。3. 不阻塞 GraphOwnerThread。 | manual_review_required | not automatically matched |
| 13 | Command Conflict Matrix | preserved_title | V1.9.1 12 Command Conflict Matrix |
| 1 | runtime conflict resolver。2. full matrix unit tests。3. documentation table。 | candidate_renamed_or_moved | candidate V1.9.1 12 Command Conflict Matrix |
| 14 | Pending Command Queue / Replay | manual_review_required | not automatically matched |
| 15 | Async Attachment Contract | preserved_title | V1.9.1 15 Async Attachment Contract |
| 1 | 不得做 IO。2. 不得等待 decoder/render/output 线程。3. 不得等待 GraphOwnerThread。4. 允许读取原子计数或 immutable snapshot。5. 如果 P5/P6 无法保证 bounded snapshot，必须改为 async QueueSnapshotEvent。 | candidate_renamed_or_moved | candidate V1.9.1 1 observer event2. query_command(command_id)3. snapshot command history window |
| 16 | GenerationStamp / Seqlock | preserved_title | V1.9.1 17 GenerationStamp / Seqlock |
| 1 | tuple_id 单调递增。2. stream_stamp_id per-session + per-stream kind 单调递增。3. 禁止 hash。4. hot path 只比较 GenerationStamp。5. hot path 不访问 GenerationTupleRegistry。 | candidate_renamed_or_moved | candidate V1.9.1 20 Hot Path Contract |
| 1 | EvidenceBuildThread 读取 observed_stamp.tuple_id。2. 读取 expected_stamp_snapshot_id。3. 展开 mismatch 字段。4. 生成完整 StaleDiscard evidence。5. tuple 已清理时生成 StaleDiscardEvidencePartial。 | manual_review_required | not automatically matched |
| 17 | Critical Event Contract | candidate_renamed_or_moved | candidate V1.9.1 18 Critical Event MPSC Ring |
| 1 | fetch_and(~consumed_mask, memory_order_acq_rel)，只清除已消费 bit。2. VersionedCriticalEventSlot，每个 slot 包含 sequence/version。3. CriticalEventRing，每个 event 有 monotonic sequence。 | manual_review_required | not automatically matched |
| 18 | C++ Memory Order Contract | manual_review_required | not automatically matched |
| 19 | Exception Safety Contract | manual_review_required | not automatically matched |
| 1 | noexcept。2. 返回 Result<T, PlaybackGraphError> 或 AsyncResult。3. 不抛异常。4. 不跨 module boundary 传播 C++ exception。 | manual_review_required | not automatically matched |
| 1 | Hot path 不分配，因此不产生 bad_alloc。2. Non-hot path 若分配失败，返回 OutOfMemory typed error。3. noexcept 函数内部不得让异常逃逸。4. 如果第三方调用可能抛异常，必须捕获并转换为 typed error。 | candidate_renamed_or_moved | candidate V1.9.1 20 Hot Path Contract |
| 20 | ABI Boundary Contract | manual_review_required | not automatically matched |
| 1 | 不跨 DLL 暴露 STL 容器。2. 不跨 DLL 传播 C++ exception。3. GenerationStamp 必须保持标准布局。4. public enum 必须显式指定 underlying type。5. public struct 不包含虚函数。6. ABI version 必须进入 capability/evidence。 | manual_review_required | not automatically matched |
| 21 | Logging Boundary | manual_review_required | not automatically matched |
| 1 | 写入 preallocated compact event。2. GraphOwnerThread 或 EvidenceBuildThread 后续转换为 evidence/log。3. DebugDeterministicPreset 下由 fake sink 收集。 | manual_review_required | not automatically matched |
| 22 | Performance Budget | manual_review_required | not automatically matched |
| 1 | emit PerformanceBudgetExceeded evidence。2. increment performance_budget_exceeded_count。3. DebugDeterministicPreset 下可使测试 fail。 | manual_review_required | not automatically matched |
| 23 | Seek / Flushing / Resource Retire | candidate_renamed_or_moved | candidate V1.9.1 24 Seek / Flushing / Commit |
| 1 | Emit SeekPhaseTimeout evidence，包含 phase、duration、operation_key。2. 进入 domain-specific recovery。3. 不回滚已完成 phase。4. 不复用 timed out operation_id。5. late completion 按 tombstone 处理。 | candidate_renamed_or_moved | candidate V1.9.1 16 Completion Queue / Timeout / Tombstone |
| 1 | 旧资源已标记 retired。2. 旧资源已进入 GraphResourceRetireQueue。3. 后台 recycle 可继续进行。 | manual_review_required | not automatically matched |
| 24 | Drain / EOS | candidate_renamed_or_moved | candidate V1.9.1 26 EOS / Drain |
| 25 | Rebuild Granularity Matrix | manual_review_required | not automatically matched |
| 26 | Physical Resource Retire / Recycle | candidate_renamed_or_moved | candidate V1.9.1 25 Resource Retire / Recycle |
| 27 | Recovery / Health Observation | manual_review_required | not automatically matched |
| 28 | System Suspend / Resume / Source Reconnect | candidate_renamed_or_moved | candidate V1.9.1 29 System Suspend / Resume |
| 29 | Evidence / Metrics / Close Flush Order | candidate_renamed_or_moved | candidate V1.9.1 31 Evidence / Metrics / Privacy |
| 30 | Privacy Redaction | manual_review_required | not automatically matched |
| 31 | P5 / P6 Attachment Contract | candidate_renamed_or_moved | candidate V1.9.1 15 Async Attachment Contract |
| 32 | Error Model | manual_review_required | not automatically matched |
| 33 | Architecture Guard | preserved_title | V1.9.1 35 Architecture Guard |
| 34 | Tests / Fake Harness | manual_review_required | not automatically matched |
| 1 | 创建全新 PlaybackSession。2. 创建全新 FakeClock。3. 创建全新 ManualEventPump。4. 创建全新 fake attachments。5. 不共享 mutable global state。6. 不依赖测试顺序。7. 不使用 sleep。 | manual_review_required | not automatically matched |
| 35 | STOP Conditions | preserved_title | V1.9.1 37 STOP Conditions |
| 36 | Exit Criteria | preserved_title | V1.9.1 38 Exit Criteria |
| 37 | Final Report 要求 | candidate_renamed_or_moved | candidate V1.9.1 39 Final Report Requirements |
| 38 | P7 不碰的后续内容 | manual_review_required | not automatically matched |
| 39 | 最终交付结论 | manual_review_required | not automatically matched |
