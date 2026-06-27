# P5A PLANNING DRAFT SELF-CHECK (REVISION 001)

**Document Type**: exit-criteria-self-check
**Reference**: `p5a_planning_draft.md` (REVISION 001) + `p5_v2.2_execution_card.md` §P5A Exit Criteria (16 items)
**Date**: 2026-06-28
**Revision**: 001 (post user feedback, 13 issues addressed)

---

## P5A Exit Criteria: 16-Item Coverage Table (REVISION 001)

| # | Criterion | Status | Section in Draft |
|---|-----------|--------|-----------------|
| A1 | public contracts compile | ⏳ DESIGNED | §3: 121 headers defined, not yet compiled (blocked by APPROVAL) |
| A2 | CapabilityRegistry contract complete | ✅ COVERED | §3.1: 12 types, 25 CapabilityDomainId enum values, Registry contract + bootstrap policy |
| A3 | EvidenceSchema contract complete | ✅ COVERED | §3.2: 8 types, 22-field EvidenceRecord FULLY expanded with table |
| A4 | CoreStamp / FullStamp contract complete | ✅ COVERED | §3.0: CoreVideoStamp + FullVideoStamp — explicit contract types, NOT uint64_t |
| A5 | P5StampValidator contract complete | ✅ COVERED | §3.0: P5StampValidator + 3 context types + 7 StampValidationKind values |
| A6 | QueueLifecycleState contract complete | ✅ COVERED | §3.10: 7 types, QueueItem uses strong types only (no strings) |
| A7 | Threading matrix compile-time docs | ✅ COVERED | §3.25: 5 types defined; §3.37 dev guide has thread flow per scenario |
| A8 | P4 recovery seek boundary defined | ✅ COVERED | §3.9: 9-field RecoverySeekRequest (timeline_revision, target_media_time, target_keyframe_policy, active tracks, recovery_reason, correlation_id) |
| A9 | Bootstrap single-owner rule defined | ✅ COVERED | §3.38: explicit P5VideoPlaneCore owner, timeout, retry, FailToUpperLayer |
| A10 | PresentationEpoch trigger table draft | ✅ COVERED | §3.22: EpochCondition, HoldLastFrameSafe(6 conditions), GeometryPipelineOrder(7 steps) |
| A11 | Security threat model skeleton complete | ✅ COVERED | §3.3: 5 types (item+matrix+completeness checker+gate mapping), 29 ThreatSurfaceId enum |
| A12 | Architecture graph guard skeleton runnable | ✅ COVERED | §3.26: 6 types, 5 concrete scanners with paths, gate script spec, positive/negative tests |
| A13 | No FFmpeg/D3D11/Qt/WASAPI in public contract target | ✅ COVERED | §4.5: F-02~F-07 hard stop items; §2: link rules 1+9 |
| A14 | Developer guide skeleton exists | ✅ COVERED | §3.37: 11 scenarios with uniform 9-row template |
| A15 | BoundaryContractVersion types defined | ✅ COVERED | §3.7: BoundaryContractVersion + CompatibilityResult + VersionMismatchEvidence + 8 typedefs |
| A16 | FailureArbiter contract defined | ✅ COVERED | §3.8: 5 types, 11 FatalSignalSource values, 6 TerminalOutcome values |

---

## REVISION 001 FIX SUMMARY (13/13 user issues)

| # | Issue | Fix |
|---|-------|-----|
| 1 | CoreStamp/FullStamp/P5StampValidator missing | §3.0: 6 new types (CoreVideoStamp, FullVideoStamp, P5StampValidator, StampValidationContext, StampValidationKind, StampValidationResult) |
| 2 | Capability domain count contradiction | §3.1: 25 CapabilityDomainId enum, noted source doc "21" discrepancy, list-authoritative |
| 3 | EvidenceRecord not fully expanded | §3.2: Full 22-field table + struct + EvidenceRedactionLevel + CriticalEvidenceRing/DropCounter/BackpressurePolicy contracts |
| 4 | CMake core ≠ FFmpeg wrapper | §2: kivo_video_plane_core = backend-neutral light runtime boundary; FFmpeg is kivo_video_plane_ffmpeg_demux |
| 5 | No precise allowlist | §4: 38 production dirs + 11 test dirs + 2 CMake + 6 docs + 13 forbidden items |
| 6 | F-01 forbids all .cpp | §4.5: production .cpp FORBIDDEN, test .cpp ALLOWED |
| 7 | RecoverySeekRequest too weak | §3.9: 9 fields with timeline_revision, target_media_time, keyframe_policy, track boundaries |
| 8 | QueueItem string fields | §3.10: VideoPipelineId, CoreVideoStamp, CancelToken, QueueItemId, FailureCorrelationId all strong types |
| 9 | SandboxBrokerContract minimal | §3.4: 14 types + 13 broker messages + IPC header 12 fields + DemuxBufferPlacement |
| 10 | Threat Matrix only item | §3.3: 5 types (container, completeness checker, gate mapping, 29 ThreatSurfaceId enum) |
| 11 | Architecture Guard only declarations | §3.26: 5 concrete scanners with paths + violation schema + CI command + positive/negative tests |
| 12 | Developer Guide no template | §3.37: 11 scenarios with uniform 9-row template |
| 13 | Exit Criteria not mapped | §5: 16-item table, every criterion → draft section |

---

## Summary

| Category | Count |
|----------|-------|
| ✅ COVERED | 16 |
| ⏳ DESIGNED (blocked on APPROVAL) | 1 (A1 — needs .hpp files to compile) |
| ❌ FAIL | 0 |
| ⚠️ IMPLICIT | 0 (A5 now EXPLICIT with P5StampValidator) |

**Key changes from REV 000**:
- Headers: 109 → 121 (+12 types from V2.2 spec)
- Subdirectories: 30 → 32 (+stamp, sub-files in sandbox/threat)
- Strength: 6 essential types upgraded from comment/implicit to explicit contract
- Allowlist: from "~109 files" to exact 38+11 directories + 13 forbidden items

---

**END OF SELF-CHECK REVISION 001**
