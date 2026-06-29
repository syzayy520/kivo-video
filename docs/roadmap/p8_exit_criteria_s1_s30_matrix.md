# KIVO-VIDEO-P8 S1-S30 EXIT CRITERIA MATRIX

**Task ID**: `KIVO-VIDEO-P8-SUBTITLE-PLANE-DESIGN-LOCK-001-V1.5.5`
**Status**: `P8H_CLOSURE_MATRIX`
**Source**: `Kivo Video P8 Subtitle Plane Design V1.5.5.docx`

| Criteria | Gate | Evidence target | Current status |
|---|---|---|---|
| S1 C header compiles as pure C | P8B-008, P8G-002, P8H-001 | `artifacts/p8/evidence/P8H-001.json` | RUNTIME_PASS |
| S2 No bare noexcept appears in C ABI header | P8B-001, P8G-002 | `artifacts/p8/evidence/P8G-002.json` | CONTRACT_PASS |
| S3 No std:: namespace appears in C ABI header | P8B-001, P8G-002 | `artifacts/p8/evidence/P8G-002.json` | CONTRACT_PASS |
| S4 Bootstrap returns KivoP8ApiStatus | P8B-002, P8C-002 | `artifacts/p8/evidence/P8C-002.json` | RUNTIME_PASS |
| S5 KIVO_P8_API_V1_REQUIRED_PREFIX_SIZE defined | P8B-002, P8C-002 | `artifacts/p8/evidence/P8C-002.json` | RUNTIME_PASS |
| S6 Function table begins with KivoP8AbiHeader | P8B-007, P8C-003 | `artifacts/p8/evidence/P8C-003.json` | RUNTIME_PASS |
| S7 All V1 required function pointers non-null | P8B-007, P8C-003 | `artifacts/p8/evidence/P8C-003.json` | RUNTIME_PASS |
| S8 KivoMutableByteSpan defined | P8B-005 | `artifacts/p8/evidence/P8B-005.json` | CONTRACT_PASS |
| S9 KivoSubtitleInputBytesView naming unified | P8B-005, P8G-002 | `artifacts/p8/evidence/P8G-002.json` | CONTRACT_PASS |
| S10 All referenced public C types defined | P8B-003..P8B-007, P8G-001 | `artifacts/p8/evidence/P8G-001.json` | CONTRACT_PASS |
| S11 FrameLease/SnapshotLease fully defined | P8B-006, P8E-007, P8F-001 | `artifacts/p8/evidence/P8F-001.json` | RUNTIME_PASS |
| S12 ObserverCallbacks fully defined | P8B-006, P8F-005 | `artifacts/p8/evidence/P8F-005.json` | RUNTIME_PASS |
| S13 StreamEndRequest fully defined | P8B-006, P8D-006 | `artifacts/p8/evidence/P8D-006.json` | RUNTIME_PASS |
| S14 Source vtable versioned with KivoP8AbiHeader | P8B-006, P8D-001 | `artifacts/p8/evidence/P8D-001.json` | RUNTIME_PASS |
| S15 ResourceAccessToken irrelevant-field rule unambiguous | P8B-006, P8D-002 | `artifacts/p8/evidence/P8D-002.json` | RUNTIME_PASS |
| S16 Discovery external preference flag exists | P8B-006, P8D-004 | `artifacts/p8/evidence/P8D-004.json` | RUNTIME_PASS |
| S17 Snapshot section table complete | P8B-006, P8F-002 | `artifacts/p8/evidence/P8F-002.json` | RUNTIME_PASS |
| S18 Metrics/string/text snapshot policy explicit | P8B-006, P8F-002, P8F-007 | `artifacts/p8/evidence/P8F-007.json` | RUNTIME_PASS |
| S19 Privacy hash/redacted identity closed | P8B-006, P8D-003, P8F-008 | `artifacts/p8/evidence/P8F-008.json` | RUNTIME_PASS |
| S20 Cancellation callback replaces volatile pointer | P8B-006, P8C-007, P8G-002 | `artifacts/p8/evidence/P8C-007.json` | RUNTIME_PASS |
| S21 UINT64_MAX cap behavior explicit | P8C-006 | `artifacts/p8/evidence/P8C-006.json` | RUNTIME_PASS |
| S22 Cold-start late packet behavior explicit | P8E-003 | `artifacts/p8/evidence/P8E-003.json` | RUNTIME_PASS |
| S23 EOS epsilon source explicit | P8E-009 | `artifacts/p8/evidence/P8E-009.json` | RUNTIME_PASS |
| S24 Hazard retry bound explicit | P8E-009 | `artifacts/p8/evidence/P8E-009.json` | RUNTIME_PASS |
| S25 ABI layout static assert gate complete | P8B-009, P8G-003 | `artifacts/p8/evidence/P8G-003.json` | CONTRACT_PASS |
| S26 Snapshot endian responsibility explicit | P8F-003 | `artifacts/p8/evidence/P8F-003.json` | RUNTIME_PASS |
| S27 Close lease tombstone policy complete | P8F-006 | `artifacts/p8/evidence/P8F-006.json` | RUNTIME_PASS |
| S28 SSA/ASS/WebVTT missing semantic sections complete | P8E-004..P8E-006 | `artifacts/p8/evidence/P8E-006.json` | RUNTIME_PASS |
| S29 All V1.5.5 tests PASS | P8G-004..P8G-008, P8H-001 | `artifacts/p8/evidence/P8H-001.json` | RUNTIME_PASS |
| S30 No P4/P5/P6/P7 frozen public contract modification | P8A-004, P8G-007, P8H-002 | `artifacts/p8/evidence/P8H-002.json` | CONTRACT_PASS |

## Corrective Addendum

The P8A corrective natural tree forbids monolithic implementation files. P8B and later gates must use the file families in `docs/roadmap/p8a_planning_draft.md`.

## Source Reconciliation Addendum

After P8 implementation and validation, P8H must re-read `C:\Users\huyasi\Downloads\Kivo Video P8 Subtitle Plane Design V1.5.5.docx` and perform a paragraph/rule-level reconciliation. Passing CTest is not enough for delivery. Any unmapped design paragraph, required test, exit criterion, final report item, closure condition, or final delivery sentence blocks commit/push until fixed or explicitly recorded as a user-approved non-goal/blocker.
