# KIVO-VIDEO-P7A-BASELINE-REPAIR-GATE-001

**Document type**: repair-gate / planning-only / no implementation
**Related gate**: `P7A-003`
**Status**: `REPAIRED_BY_REAL_V1.9_BASELINE_RECOVERY`
**Generated**: 2026-06-29

## 1. Failure Summary

`P7A-003` failed because P7 V1.9.1 requires a V1.9 baseline document and rule manifest, but neither file exists in the approved workspace.

Required paths:

```text
docs/design/p7/KIVO-VIDEO-P7-PLAYBACK-GRAPH-INTEGRATION-DESIGN-LOCK-001-V1.9.md
docs/design/p7/KIVO-VIDEO-P7-PLAYBACK-GRAPH-INTEGRATION-DESIGN-LOCK-001-V1.9.rule_manifest.yaml
```

Evidence:

```text
artifacts/p7/evidence/P7A-003.json
```

## 2. Why This Blocks P7

P7 V1.9.1 section 2 requires V1.9 strict-super-set verification, baseline hashes, and a missing-carryover report. P7 V1.9.1 exit criterion `G52` requires the V1.9 baseline manifest/hash to exist.

Without the real V1.9 baseline:

- V1.9.1 strict-super-set status is not verifiable.
- `v1_9_doc_hash_sha256` is not verifiable.
- `v1_9_rule_manifest_hash_sha256` is not verifiable.
- Missing, renamed, moved, corrected, or removed V1.9 rules cannot be audited.
- P7 implementation would start from an unproven design base.

## 3. Allowed Repair Inputs

One of these must happen before P7 may continue:

1. The real V1.9 baseline Markdown file is placed at the required `docs/design/p7/...V1.9.md` path.
2. The real V1.9 rule manifest YAML is placed at the required `docs/design/p7/...V1.9.rule_manifest.yaml` path.
3. A task-specific baseline commit is provided and verified to contain the required files.
4. The user explicitly revises P7 V1.9.1 requirements and accepts a named `G52` deferral. This must be recorded as an accepted deferral, not as PASS.

## 4. Forbidden Repair Actions

- Do not fabricate a V1.9 document from V1.9.1.
- Do not infer the rule manifest from memory.
- Do not mark `G52` PASS without real hashes.
- Do not create placeholder baseline files.
- Do not continue into P7B implementation while this repair gate is unresolved.
- Do not use any working directory outside `C:\kivo video`.

## 5. Verification Commands

After repair input is available, rerun:

```powershell
Test-Path -LiteralPath 'C:\kivo video\docs\design\p7\KIVO-VIDEO-P7-PLAYBACK-GRAPH-INTEGRATION-DESIGN-LOCK-001-V1.9.md'
Test-Path -LiteralPath 'C:\kivo video\docs\design\p7\KIVO-VIDEO-P7-PLAYBACK-GRAPH-INTEGRATION-DESIGN-LOCK-001-V1.9.rule_manifest.yaml'
Get-FileHash -Algorithm SHA256 -LiteralPath 'C:\kivo video\docs\design\p7\KIVO-VIDEO-P7-PLAYBACK-GRAPH-INTEGRATION-DESIGN-LOCK-001-V1.9.md'
Get-FileHash -Algorithm SHA256 -LiteralPath 'C:\kivo video\docs\design\p7\KIVO-VIDEO-P7-PLAYBACK-GRAPH-INTEGRATION-DESIGN-LOCK-001-V1.9.rule_manifest.yaml'
git -c safe.directory='C:/kivo video' merge-base HEAD origin/main
```

If `merge-base` still fails, the task must provide a baseline commit explicitly.

## 6. Resume Criteria

P7 may resume at `P7A-003` only when:

- both required V1.9 files exist,
- both hashes are recorded,
- baseline commit is recorded or explicitly supplied,
- missing-carryover report can be generated,
- `P7A-003.json` is updated from `FAIL` to a new audited result only after rerunning the gate.

## 7. Professional Decision

The real V1.9 baseline files were initially unavailable in the approved workspace and were not found in local git refs, tags, stash, ignored files, or current workspace searches. The user then provided the real V1.9 DOCX from Downloads.

Decision:

- Do not fabricate V1.9 content.
- Extract V1.9 mechanically into the required project-local baseline Markdown.
- Generate the required V1.9 rule manifest from V1.9 headings.
- Keep the V1.9.1 DOCX as the authoritative P7 implementation baseline.
- Use V1.9 only for carryover audit.
- Supersede the temporary `P7A-003R` baseline replacement decision.

This repair allows P7A to continue. It does not by itself prove full V1.9 carryover; manual-review rows in the carryover report must be resolved before final P7 closure.

## 8. Current Next Action

Continue from `P7A-004` with V1.9.1 as the single authoritative implementation baseline and V1.9 as the historical carryover audit source.

**END OF P7A BASELINE REPAIR GATE**
