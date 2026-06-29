# KIVO-VIDEO-P7A-BASELINE-REPLACEMENT-DECISION-001

**Document type**: design-history repair decision
**Related failed gate**: `P7A-003`
**Repair evidence**: `artifacts/p7/evidence/P7A-003R.json`
**Status**: `SUPERSEDED_BY_REAL_V1.9_BASELINE_RECOVERY`
**Generated**: 2026-06-29

## 1. Decision

This temporary decision is superseded because the user provided the real V1.9 DOCX:

```text
C:\Users\huyasi\Downloads\Kivo Video P7 Playback Graph Integration Design V1.9.docx
```

`C:\kivo video\.codex\Kivo Video P7 Playback Graph Integration Design V1.9.1.docx` remains the single authoritative implementation baseline for P7. V1.9 is now available only for historical carryover audit.

## 2. Rationale

P7 V1.9.1 requires a V1.9 baseline manifest to prove strict carryover from V1.9. The required files are absent from the workspace and were not found in the local repository refs, tags, stash, ignored files, `.codex`, `.codebuddy`, `docs`, or `artifacts` searches. Remote lookup was attempted but GitHub was unreachable from this environment.

Blocking all P7 implementation on a missing historical draft would have prevented progress without increasing implementation correctness. Fabricating the files would have been worse: it would create false evidence and poison all later gates.

The professional repair was temporary. It is now superseded by real V1.9 recovery recorded in `P7A-003.json`.

## 3. What This Does Not Prove

This decision does not prove:

- V1.9.1 is a strict superset of V1.9.
- every V1.9 rule was preserved, renamed, moved, corrected, or removed-as-invalid.
- the V1.9 document hash.
- the V1.9 rule manifest hash.

These are now seeded by real V1.9 baseline artifacts, but the detailed carryover mapping still requires manual review before final P7 closure.

## 4. What This Allows

The superseding real-baseline recovery allows:

- P7A-004 dependency boundary inventory.
- P7A-005 planning draft and natural family tree.
- P7A-006 authorization record.
- Later implementation gates based on V1.9.1 only, with V1.9 used for carryover audit.

## 5. Required Reporting Rule

Every P7 closure report must state:

Use the reporting rule in `artifacts/p7/evidence/P7A-003.json` and `docs/roadmap/p7a_v1.9_to_v1.9.1_carryover_report.md`.

## 6. Forbidden Follow-up Claims

- Do not claim full V1.9 carryover PASS until the carryover report manual-review rows are resolved.
- Do not backfill V1.9 files from V1.9.1 content.
- Do not remove `P7A-003.json`.

## 7. Resume Point

Resume P7 at `P7A-004`; this document is historical context only.

**END OF P7A BASELINE REPLACEMENT DECISION**
