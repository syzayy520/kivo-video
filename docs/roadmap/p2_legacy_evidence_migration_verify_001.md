# KIVO-VIDEO-P2-LEGACY-EVIDENCE-MIGRATION-VERIFY-001 Planning

## Task Identity
- **Full Task ID**: KIVO-VIDEO-P2-LEGACY-EVIDENCE-MIGRATION-VERIFY-001
- **Evidence File ID**: LEM-001
- **Family**: governance / evidence-audit
- **Depends On**: P2-REDLINE-GATE-FOUNDATION-001 (honest FAIL state accepted)
- **Goal**: Audit and re-run legacy evidence migration so that all historical fields are preserved without deletion, while keeping the redline gate in an honest FAIL state.

## Scope
- **In Scope**:
  - 51 legacy evidence files under `artifacts/p2/evidence/*.json` (all non-RGF files)
  - Migration tooling under `backend/tools/governance/redline_gate/`
  - Audit reports and evidence artifacts
  - Planning and checklist documentation
- **Out of Scope**:
  - `backend/src/**` (no code changes)
  - Local Runtime / E2E / Soak / D3D11 / WASAPI tests
  - P2-SOURCE-CORE implementation
  - RGF-*.json files (forbidden to modify)
  - Redline gate logic changes intended to make the gate pass

## Requirements
1. **Field Preservation**
   - Do NOT delete historical fields such as `runtime_verified_timestamp`, `provider_matrix_impact`, `task_name`, `family`, or any other pre-existing evidence fields.
   - Migrate removed/moved fields to at least one of:
     - `legacy_original` — complete original JSON snapshot before migration
     - `legacy_fields` — key-value map of historical fields no longer at top level
     - `implementation.legacy_*` — implementation-specific legacy values
2. **RGF Protection**
   - Confirm RGF-001.json through RGF-005.json remain 0 changed relative to the pre-migration baseline.
3. **Before/After Report**
   - For each of the 51 evidence files, report the set of fields preserved and where they are preserved.
4. **Violation Classification**
   - Run the production redline scan and classify the remaining violations into:
     - Real redline violations that must be remediated before PASS
     - Documentation false positives where the claim appears in a negated/forbidden context
5. **Honest Status**
   - Self-test must remain PASS.
   - Production scan must remain FAIL until all real violations are fixed.
   - No attempt to disguise FAIL as PASS.

## Acceptance Criteria
- [ ] RGF-*.json files are identical to pre-migration baseline
- [ ] 51 legacy evidence files have all historical fields preserved in `legacy_original`/`legacy_fields`/`implementation.legacy_*`
- [ ] All 51 evidence files validate against `p2-evidence-v8` schema (or their declared schema)
- [ ] Before/after field retention report generated
- [ ] Production scan violation classification report generated
- [ ] Evidence file LEM-001.json created and schema-valid
- [ ] `backend_task_checklist.md` updated with LEM-001 DONE
- [ ] No changes to `backend/src/**` or Local Runtime tests

## Execution Steps
1. Capture pre-migration baseline from git commit `e02a856^` (commit before migration `011c270`).
2. Write new migration script `migrate_legacy_evidence_preserve.py` that preserves historical fields.
3. Restore original evidence files from baseline.
4. Run new migration script.
5. Verify RGF-*.json unchanged.
6. Generate `legacy_evidence_field_retention_report.json`.
7. Run redline production scan and generate `violation_classification_report.json`.
8. Create `artifacts/p2/evidence/LEM-001.json`.
9. Update `docs/roadmap/backend_task_checklist.md`.
10. Update memory files.

## Risk
- Restoring original evidence files may temporarily break schema validation; this is expected and repaired by the new migration.
- If a field is both historical and part of the new schema, it stays at top level and is also copied to `legacy_fields` for audit transparency.
- No attempt will be made to hide production scan FAIL status.

## Status Vocabulary
- This task is a **governance/audit** task.
- Final classification can be **CONTRACT_PASS** (audit completed, migration preserved fields, production scan still FAIL honestly).
- It is **NOT** a claim that P2-REDLINE-GATE is PASS.
