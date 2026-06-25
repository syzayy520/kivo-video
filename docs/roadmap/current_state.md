# Current State

Updated: 2026-06-25

## Done

- Local Codex memory created for the Kivo Video iron gate.
- Backend goal persisted in `.codex/memory/backend-goal.md`.
- Detailed execution checklist created at `docs/roadmap/backend_task_checklist.md`.
- P2 Real Playback Backend Implementation Spec V1.5 added to `docs/roadmap/backend_task_checklist.md`.
- P2 Real Local Closure V10 added to `docs/roadmap/backend_task_checklist.md` as `P2-REAL-LOCAL-CLOSURE-V10`, backed by PDF, pasted text, and docx sources.
- P2 Remaining Full Coverage Roadmap V8 added to `docs/roadmap/backend_task_checklist.md` as `P2-REMAINING-COVERAGE-V8`; next executable child task is `P2-LOCAL-REPAIR-DELTA-001`.
- V8 direct execution cards created at `docs/roadmap/p2_remaining_coverage_v8_execution_cards.md`; next assistant should execute cards in order starting at `V8-001`.
- Latest checklist cross-check found a V8/V10 issue: some current V10 runtime claims mention stub/simulator evidence, which V8 forbids as `RUNTIME_PASS`; next work starts with audit, downgrade, or real-runtime repair before remote/provider work.
- Latest governance verification: V8-001 through V8-009 completed. V8-001 cleaned root, V8-002 audited consistency, V8-003 protected work, V8-004 mapped families, V8-005 enforced claim vocabulary, V8-006 enforced status vocabulary, V8-007 enforced evidence paths, V8-008 defined provider matrix schema, V8-009 locked final gates. All evidence in artifacts/p2/evidence/.
- P2-LOCAL-REPAIR-DELTA-001 completed: LRD-001 through LRD-013 all DONE. Evidence in artifacts/p2/evidence/LRD-001.json through LRD-013.json. Planning documents in docs/roadmap/p2_local_repair_delta_*.md. Overall classification: CONTRACT_PASS (real FFmpeg foundation proven; downstream stages are correct stubs but cannot be RUNTIME_PASS without real hardware). Next task: SGF-001 Define p2-evidence-v8 schema.
- V10-014 lifecycle abort popup fixed: `kivo_lifecycle_test` now follows the real startup state-machine path and passes; full `ctest --preset vs-debug --output-on-failure --timeout 30` passes 74/74.
- Natural family tree created in `docs/p0/core-architecture/natural_family_tree.md`.
- First 12 P0 contract docs created.
- Second batch P0 docs created for subtitle, audio, inspector, and diagnostic manifest.
- CMake project entry created.
- C++20 contract headers created for the V1.1 required contract surface.
- ContractRegistry implementation created.
- P1 fake direct-play scenario created.
- Minimal C++ fake session test created.
- Playback invariant checker created.
- Diagnostic redaction gate created.
- Engine API P1 boundary created for command queue, event subscription, state snapshot store, and cancellation token.
- SessionStore and DecisionLedgerStore created for P1 in-memory query/export.
- ReplayManifest created and wired into fake Engine API scenario.
- PlaybackCore fake state machine created with invariant-backed transitions.
- GraphCore fake runtime created with edge capacity, lifecycle, backpressure, seek barrier, flush generation, and stop invariants.
- P1 media pipeline contract shells created for fake cache read, stream inventory, and decode path.
- Color/HDR/Presenter fake services created.
- Audio/Subtitle fake services created.
- Governance gate script created and passed.
- P2-001A/B/C completed: 5 design docs, 20 minimal slice headers, 10 placeholder headers, governance gate updated with P2 checks.

## Verification

- `backend/tools/governance/verify_p1_gates.py`: PASS (P0/P1/P2 governance gates).
- `cmake --preset ninja-debug`: PASS.
- `cmake --build --preset ninja-debug`: PASS (70 targets).
- `ctest --preset ninja-debug --output-on-failure`: PASS, 12/12 tests.
- `cmake --build --preset vs-debug`: PASS.
- `ctest --preset vs-debug --output-on-failure --timeout 30`: PASS, 74/74 tests.
- Latest `backend/tools/governance/verify_p1_gates.py`: blocked by root temporary outputs (build_output.txt, tmp/). V8-001 cleaned root: build_output.txt and tmp/ removed, workspace documented, evidence generated at artifacts/p2/evidence/V8-001.json. V8-002 audit completed: 4 V10 items downgraded to CONTRACT_PASS, 18 remain RUNTIME_PASS. Evidence at artifacts/p2/evidence/V8-002.json. V8-003 protected work documented: FFmpeg skeleton, presenter abort fix, D3D11 skip semantics, wasapi_internal.hpp, d3d11_internal.hpp, FfmpegFrameResult frame data fields. Evidence at artifacts/p2/evidence/V8-003.json. V8-004 natural family tree gate completed: all P2 remaining families mapped to existing backend natural family tree style. Family names match task IDs. No dumping-ground families. Dependency direction documented. Evidence at artifacts/p2/evidence/V8-004.json. V8-005 allowed/forbidden claims gate completed: 6 forbidden terms scanned, 0 premature completion claims found. Allowed claim vocabulary documented. Evidence at artifacts/p2/evidence/V8-005.json. V8-006 status vocabulary gate completed: all 5 V8 evidence files use only standard V8 status vocabulary (RUNTIME_PASS, CONTRACT_PASS). No non-standard statuses found. Evidence at artifacts/p2/evidence/V8-006.json.

## Next Work

Follow `docs/roadmap/backend_task_checklist.md`.

Immediate next task:

1. Open `docs/roadmap/p2_remaining_coverage_v8_execution_cards.md`.
2. Execute `SGF-001` Define `p2-evidence-v8` schema.
3. Execute `SGF-002` Define `p2-inspector-v8` schema.
4. Execute `SGF-003` Define `p2-decision-ledger-v8` schema.
5. Continue with `SGF-004` through `SGF-008` for schema foundation completion.
