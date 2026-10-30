# P4 Type Ownership Table

| Type Name | Authority | P4 Owns Definition | Header | Hot Path | Alloc | Cross-Thread |
|-----------|-----------|-------------------|--------|----------|-------|-------------|
| Milliseconds | p4 | yes | primitives/value_types.hpp | yes | no | copy |
| Microseconds | p4 | yes | primitives/value_types.hpp | yes | no | copy |
| Nanoseconds | p4 | yes | primitives/value_types.hpp | yes | no | copy |
| Bytes | p4 | yes | primitives/value_types.hpp | yes | no | copy |
| BitsPerSecond | p4 | yes | primitives/value_types.hpp | yes | no | copy |
| BytesPerSecond | p4 | yes | primitives/value_types.hpp | yes | no | copy |
| Count | p4 | yes | primitives/value_types.hpp | yes | no | copy |
| Ratio | p4 | yes | primitives/ratio.hpp | yes | no | copy |
| PlaybackRateRatio | p4 | yes | rate/playback_rate_ratio.hpp | yes | no | copy |
| MonotonicTimestamp | p4 | yes | primitives/time_types.hpp | yes | no | copy |
| Deadline | p4 | yes | primitives/time_types.hpp | yes | no | copy |
| ReadDeadline | p4 | yes | primitives/time_types.hpp | yes | no | copy |
| PullDeadline | p4 | yes | primitives/time_types.hpp | yes | no | copy |
| MediaTimeMs | p4 | yes | primitives/time_types.hpp | yes | no | copy |
| MediaDeltaMs | p4 | yes | primitives/time_types.hpp | yes | no | copy |
| SourceOffset | p4 | yes | primitives/source_range.hpp | yes | no | copy |
| SourceRange | p4 | yes | primitives/source_range.hpp | yes | no | copy |
| StreamType | p4 | yes | primitives/stream_type.hpp | yes | no | copy |
| SessionId | p4 | yes | revision/revision_master_model.hpp | yes | no | copy |
| CommandId | p4 | yes | revision/revision_master_model.hpp | yes | no | copy |
| ReaderInstanceId | p4 | yes | revision/revision_master_model.hpp | yes | no | copy |
| MemoryBlockId | p4 | yes | revision/revision_master_model.hpp | yes | no | copy |
| MemoryPoolId | p4 | yes | revision/revision_master_model.hpp | yes | no | copy |
| TrackSwitchFenceId | p4 | yes | revision/revision_master_model.hpp | yes | no | copy |
| FeedbackId | p4 | yes | revision/revision_master_model.hpp | yes | no | copy |
| EventSubscriptionId | p4 | yes | revision/revision_master_model.hpp | yes | no | copy |
| SessionGeneration | p4 | yes | revision/revision_master_model.hpp | yes | no | copy |
| CommandSequence | p4 | yes | revision/revision_master_model.hpp | yes | no | copy |
| ReadEpoch | p4 | yes | revision/revision_master_model.hpp | yes | no | copy |
| SeekEpoch | p4 | yes | revision/revision_master_model.hpp | yes | no | copy |
| RecoveryEpoch | p4 | yes | revision/revision_master_model.hpp | yes | no | copy |
| StreamRevision | p4 | yes | revision/revision_master_model.hpp | yes | no | copy |
| TimelineRevision | p4 | yes | revision/revision_master_model.hpp | yes | no | copy |
| P4RuntimePolicyRevision | p4 | yes | revision/revision_master_model.hpp | yes | no | copy |
| CallbackToken | p4 | yes | revision/callback_token.hpp | yes | no | copy |
| RevisionStamp | p4 | yes | revision/revision_stamp.hpp | yes | no | copy |
| SnapshotStamp | p4 | yes | revision/snapshot_stamp.hpp | yes | no | immutable |
| SessionState | p4 | yes | state/session_state.hpp | yes | no | copy |
| PlaybackHandoffContract | p3 | no (read-only) | p3/playback_handoff/handoff_contracts.hpp | no | n/a | n/a |
| PlaybackCandidateSet | p3 | no (read-only) | p3/playback_decision/decision_contracts.hpp | no | n/a | n/a |
| SourceSession | p3 | no (read-only) | p3/sessions/sessions.hpp | no | n/a | n/a |
| ProbeEvidence | p3 | no (read-only) | p3/media_probe/probe_contracts.hpp | no | n/a | n/a |
| BufferRequirement | p3 | no (read-only) | p3/platform_contracts/platform_contracts.hpp | no | n/a | n/a |
| RuntimeRevisionResult | p3 | no (read-only) | p3/playback_decision/decision_contracts.hpp | no | n/a | n/a |
